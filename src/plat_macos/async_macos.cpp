// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include "async_macos.hpp"

#include "os/async_internal.hpp"

#include <array>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <unordered_map>

#include <IOKit/IOReturn.h>
#include <magic_enum.hpp>
#include <sys/event.h>

#include "os/async.hpp"
#include "os/errcheck.hpp"
#include "sockets/enums.hpp"

using CompletionQueue = std::queue<Async::CompletionResult*>;

struct SocketQueue {
    CompletionQueue pendingReads;
    CompletionQueue pendingWrites;
};

using SocketQueueMap = std::unordered_map<uint64_t, SocketQueue>;

static constexpr auto ASYNC_CANCEL = 1UL << 33;

static int kq = -1;

// Operation queues for sockets
// kqueue and IOBluetooth only handle notifications for I/O, so completion queues must be managed manually.
static std::array<SocketQueueMap, magic_enum::enum_count<SocketTag>()> sockets;
static std::mutex mapMutex;

// Bluetooth read results
static std::unordered_map<uint64_t, std::queue<std::optional<std::string>>> bluetoothReads;

// Gets the map of IDs to completion queues for a socket tag.
static SocketQueueMap& getQueueMap(SocketTag tag) {
    return sockets[magic_enum::enum_integer(tag)];
}

// Gets the queue of pending operations for a socket/Bluetooth channel.
static CompletionQueue& getPendingQueue(uint64_t id, SocketTag tag, Async::IOType ioType) {
    auto& queue = getQueueMap(tag)[id];

    return (ioType == Async::IOType::Send) ? queue.pendingWrites : queue.pendingReads;
}

// Adds a pending operation.
static void addPending(uint64_t id, SocketTag tag, Async::IOType ioType, Async::CompletionResult& result) {
    std::scoped_lock lock{ mapMutex };
    getPendingQueue(id, tag, ioType).push(&result);
}

// Pops and returns a pending operation.
static std::optional<Async::CompletionResult*> popPending(uint64_t id, SocketTag tag, Async::IOType ioType) {
    std::scoped_lock lock{ mapMutex };

    auto& queue = getPendingQueue(id, tag, ioType);
    if (queue.empty()) return std::nullopt;

    auto front = queue.front();
    queue.pop();
    return front;
}

// Pops and cancels a pending operation.
static std::optional<Async::CompletionResult*> cancelOne(uint64_t id, SocketTag tag, Async::IOType ioType) {
    // Completion result pointing to suspended coroutine
    auto pending = popPending(id, tag, ioType);
    if (!pending) return std::nullopt;

    // Set error and return coroutine handle
    auto& result = *pending;
    result->error = ECANCELED;
    return result;
}

// Removes kqueue events for a socket.
static void deleteKqueueEvents(int id) {
    std::array<struct kevent, 2> events;

    EV_SET(&events[0], id, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    EV_SET(&events[1], id, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);

    kevent(kq, events.data(), events.size(), nullptr, 0, nullptr);
}

// Processes user events from the kqueue.
static Async::CompletionResult& handleUserEvent(const struct kevent& event) {
    if (event.ident == Async::Internal::ASYNC_INTERRUPT) {
        // Exit thread if requested
        throw Async::Internal::WorkerInterruptedError{};
    } else if (event.ident & ASYNC_CANCEL) {
        using enum SocketTag;
        using enum Async::IOType;

        // Extract file descriptor from identifier and remove kqueue events
        int id = event.ident & 0xFFFFFFFFUL;
        deleteKqueueEvents(id);

        // Cancel receive and send operations in order
        // The cancel operation can be picked up by multiple threads so each handles one event until they are all
        // drained.
        if (const auto recvCancel = cancelOne(id, IP, Receive)) return **recvCancel;

        if (const auto sendCancel = cancelOne(id, IP, Send)) return **sendCancel;

        // At this point, there are no more pending operations and the cancellation request can be deleted
        struct kevent deleteEvent {};

        EV_SET(&deleteEvent, ASYNC_CANCEL | id, EVFILT_USER, EV_DELETE, 0, 0, nullptr);
        kevent(kq, &deleteEvent, 1, nullptr, 0, nullptr);
    }

    static Async::CompletionResult noopResult;
    noopResult.coroHandle = std::noop_coroutine();
    return noopResult;
}

void Async::Internal::init(unsigned int) {
    kq = call(FN(kqueue));
}

void Async::Internal::stopThreads(unsigned int) {
    // Submit a single event to wake up all threads
    struct kevent event {};

    EV_SET(&event, ASYNC_INTERRUPT, EVFILT_USER, EV_ADD, NOTE_TRIGGER, 0, nullptr);
    kevent(kq, &event, 1, nullptr, 0, nullptr);
}

void Async::Internal::cleanup() {
    // Delete the interruption user event
    struct kevent event {};

    EV_SET(&event, ASYNC_INTERRUPT, EVFILT_USER, EV_DELETE, 0, 0, nullptr);
    kevent(kq, &event, 1, nullptr, 0, nullptr);
}

Async::CompletionResult Async::Internal::worker() {
    // Wait for new event
    struct kevent event {};

    call(FN(kevent, kq, nullptr, 0, &event, 1, nullptr));

    if (event.filter == EVFILT_USER) return handleUserEvent(event);

    // Get I/O type from user data pointer
    auto ioTypeInt = static_cast<int>(std::bit_cast<uint64_t>(event.udata));
    auto ioType = magic_enum::enum_cast<IOType>(ioTypeInt);

    // Pop an event from the queue and get its completion result
    auto& result = **popPending(event.ident, SocketTag::IP, ioType.value());

    // Set error and result
    if (event.flags & EV_EOF) result.error = static_cast<System::ErrorCode>(event.fflags);
    else result.res = static_cast<int>(event.data);

    return result;
}

void Async::submitKqueue(int ident, IOType ioType, CompletionResult& result) {
    // The EV_ONESHOT flag will delete the event once it is retrieved in one of the threads.
    // This ensures only one thread will wake up to handle the event.
    struct kevent event {};

    // Pass I/O type as user data pointer
    auto typeData = std::bit_cast<void*>(static_cast<uint64_t>(magic_enum::enum_integer(ioType)));

    EV_SET(&event, ident, (ioType == IOType::Send) ? EVFILT_WRITE : EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, typeData);
    call(FN(kevent, kq, &event, 1, nullptr, 0, nullptr));

    // Add to pending queue if the submit call succeeded
    addPending(ident, SocketTag::IP, ioType, result);
}

void Async::cancelPending(int fd) {
    struct kevent event {};

    // EV_ONESHOT is not set so each thread can handle one pending operation
    // The file descriptor is used in "ident" so events can remain unique.
    EV_SET(&event, ASYNC_CANCEL | fd, EVFILT_USER, EV_ADD, NOTE_TRIGGER, 0, nullptr);
    call(FN(kevent, kq, &event, 1, nullptr, 0, nullptr));
}

void Async::submitIOBluetooth(uint64_t id, IOType ioType, CompletionResult& result) {
    addPending(id, SocketTag::BT, ioType, result);
}

bool Async::bluetoothComplete(uint64_t id, IOType ioType, IOReturn status) {
    auto pending = popPending(id, SocketTag::BT, ioType);
    if (!pending) return false;

    auto& result = **pending;

    // Resume caller
    result.error = status;
    result.coroHandle();
    return true;
}

void Async::bluetoothReadComplete(uint64_t id, const char* data, size_t dataLen) {
    bluetoothReads[id].emplace(std::in_place, data, dataLen);

    bluetoothComplete(id, IOType::Receive, kIOReturnSuccess);
}

void Async::bluetoothClosed(uint64_t id) {
    bluetoothReads[id].emplace();

    // Close events are determined by the receive result, resume the first read operation in the queue
    bluetoothComplete(id, IOType::Receive, kIOReturnSuccess);
}

std::optional<std::string> Async::getBluetoothReadResult(uint64_t id) {
    auto data = bluetoothReads[id].front();
    bluetoothReads[id].pop();
    return data;
}

void Async::clearBluetoothDataQueue(uint64_t id) {
    bluetoothReads.erase(id);
}

void Async::bluetoothCancel(uint64_t id) {
    // Loop through all pending events and send the "aborted" signal

    // clang-format doesn't recognize semicolons after loops
    // clang-format off
    while (bluetoothComplete(id, IOType::Send, kIOReturnAborted));
    while (bluetoothComplete(id, IOType::Receive, kIOReturnAborted));
    // clang-format on
}
#endif
