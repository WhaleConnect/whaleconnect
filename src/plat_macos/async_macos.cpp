// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// kqueue and IOBluetooth only handle notifications for I/O, so completion queues must be managed manually.

#if OS_MACOS
#include "async_macos.hpp"

#include "os/async_internal.hpp"

#include <array>
#include <optional>
#include <queue>
#include <stdexcept>
#include <unordered_map>

#include <IOKit/IOReturn.h>
#include <magic_enum.hpp>
#include <sys/event.h>

#include "os/async.hpp"
#include "os/errcheck.hpp"
#include "os/error.hpp"

using CompletionQueue = std::queue<Async::CompletionResult*>;
using OptCompletionResult = std::optional<Async::CompletionResult*>;

struct SocketQueue {
    CompletionQueue pendingReads;
    CompletionQueue pendingWrites;
};

using SocketQueueMap = std::unordered_map<uint64_t, SocketQueue>;

// User event idenfitiers (combined with socket file descriptors)
static constexpr auto ASYNC_CANCEL = 1UL << 33; // Cancel all operations
static constexpr auto ASYNC_ADD = 1UL << 34;    // Add socket to queue

// Bitmask to extract file descriptors from the above idenfifiers
static constexpr auto SOCKET_ID_MASK = 0xFFFFFFFFUL;

// kqueue file descriptors for each worker thread
// Communication from the main thread to the worker threads should be done through kqueue user events.
static std::vector<int> kqs;
static int currentKqueueIdx = 0;

// Queue for Bluetooth sockets
static SocketQueueMap btSockets;

// Bluetooth read results
static std::unordered_map<uint64_t, std::queue<std::optional<std::string>>> btReads;

// Gets the queue of pending operations for a socket/Bluetooth channel.
static CompletionQueue& getPendingQueue(uint64_t id, SocketQueueMap& map, Async::IOType ioType) {
    auto& queue = map[id];

    return (ioType == Async::IOType::Send) ? queue.pendingWrites : queue.pendingReads;
}

// Adds a pending operation.
static void addPending(uint64_t id, SocketQueueMap& map, Async::IOType ioType, Async::CompletionResult& result) {
    getPendingQueue(id, map, ioType).push(&result);
}

// Pops and returns a pending operation.
static OptCompletionResult popPending(uint64_t id, SocketQueueMap& map, Async::IOType ioType) {
    auto& queue = getPendingQueue(id, map, ioType);
    if (queue.empty()) return std::nullopt;

    auto front = queue.front();
    queue.pop();
    return front;
}

// Pops and cancels a pending operation.
static OptCompletionResult cancelOne(uint64_t id, SocketQueueMap& map, Async::IOType ioType) {
    // Completion result pointing to suspended coroutine
    auto pending = popPending(id, map, ioType);
    if (!pending) return std::nullopt;

    // Set error and return coroutine handle
    auto& result = *pending;
    result->error = ECANCELED;
    return result;
}

// Removes kqueue events for a socket.
static void deleteKqueueEvents(int kq, int id) {
    std::array<struct kevent, 2> events;

    EV_SET(&events[0], id, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    EV_SET(&events[1], id, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);

    kevent(kq, events.data(), events.size(), nullptr, 0, nullptr);
}

// Processes cancel events from the kqueue.
static void handleCancel(int kq, SocketQueueMap& sockets, const struct kevent& event) {
    using enum Async::IOType;

    // Extract file descriptor from identifier and remove kqueue events
    int id = event.ident & SOCKET_ID_MASK;
    deleteKqueueEvents(kq, id);

    // Cancel receive and send operations in order
    while (const auto recvCancel = cancelOne(id, sockets, Receive)) (*recvCancel)->coroHandle();

    while (const auto sendCancel = cancelOne(id, sockets, Send)) (*sendCancel)->coroHandle();
}

void Async::Internal::init(unsigned int numThreads) {
    // Populate the vector of kqueues
    kqs.reserve(numThreads);

    for (unsigned int i = 0; i < numThreads; i++) kqs.push_back(call(FN(kqueue)));
}

void Async::Internal::stopThreads(unsigned int) {
    // Submit events to wake up all threads
    for (auto kq : kqs) {
        struct kevent event {};

        EV_SET(&event, ASYNC_INTERRUPT, EVFILT_USER, EV_ADD | EV_ONESHOT, NOTE_TRIGGER, 0, nullptr);
        kevent(kq, &event, 1, nullptr, 0, nullptr);
    }
}

void Async::Internal::cleanup() {
    // No cleanup needed
}

void Async::Internal::worker(int threadNum) {
    SocketQueueMap sockets;

    const int kq = kqs[threadNum];
    while (true) {
        struct kevent event {};

        // Wait for new event
        try {
            call(FN(kevent, kq, nullptr, 0, &event, 1, nullptr));
        } catch (const System::SystemError&) {
            continue;
        }

        // Handle user events
        if (event.filter == EVFILT_USER) {
            const auto ident = event.ident;

            if (ident == ASYNC_INTERRUPT) break;

            if (ident & ASYNC_ADD) {
                // Add completion result to queue
                auto id = ident & SOCKET_ID_MASK;
                auto ioType = magic_enum::enum_cast<IOType>(event.fflags & NOTE_FFLAGSMASK).value();
                auto& result = *std::bit_cast<CompletionResult*>(event.udata);

                addPending(id, sockets, ioType, result);
            } else if (ident & ASYNC_CANCEL) {
                // Cancel pending operations
                handleCancel(kq, sockets, event);
            }

            continue;
        }

        // Get I/O type from user data pointer
        auto ioTypeInt = static_cast<int>(std::bit_cast<uint64_t>(event.udata));
        auto ioType = magic_enum::enum_cast<IOType>(ioTypeInt).value();

        // Pop an event from the queue and get its completion result
        auto resultOpt = popPending(event.ident, sockets, ioType);
        if (!resultOpt) continue;

        auto& result = **resultOpt;

        // Set error and result
        if (event.flags & EV_EOF) result.error = static_cast<System::ErrorCode>(event.fflags);
        else result.res = static_cast<int>(event.data);

        result.coroHandle();
    }
}

void Async::submitKqueue(int ident, IOType ioType, CompletionResult& result) {
    // The EV_ONESHOT flag will delete the event once it is retrieved in one of the threads.
    // This ensures only one thread will wake up to handle the event.
    std::array<struct kevent, 3> events{};
    int ioTypeInt = magic_enum::enum_integer(ioType);

    // Pass I/O type as user data pointer
    auto typeData = std::bit_cast<void*>(static_cast<uint64_t>(ioTypeInt));
    int filt = (ioType == IOType::Send) ? EVFILT_WRITE : EVFILT_READ;

    // Add and disable the I/O filter
    // If there's a problem with the file descriptor, kevent will exit early.
    EV_SET(&events[0], ident, filt, EV_ADD | EV_DISABLE, 0, 0, nullptr);

    // Request to add the operation to the pending queue
    // Early exiting of kevent will prevent dangling entries in the thread's queue if the socket's I/O filter didn't
    // make it into the kqueue.
    EV_SET(&events[1], ASYNC_ADD | ident, EVFILT_USER, EV_ADD | EV_ONESHOT, NOTE_TRIGGER | ioTypeInt, 0, &result);

    // Enable the I/O filter once the pending queue has been modified
    EV_SET(&events[2], ident, filt, EV_ENABLE | EV_ONESHOT, 0, 0, typeData);

    call(FN(kevent, kqs[currentKqueueIdx], events.data(), events.size(), nullptr, 0, nullptr));

    // Cycle through all worker threads
    currentKqueueIdx = (currentKqueueIdx + 1) % kqs.size();
}

void Async::cancelPending(int fd) {
    for (auto kq : kqs) {
        struct kevent event {};

        // The file descriptor is used in "ident" so events can remain unique
        EV_SET(&event, ASYNC_CANCEL | fd, EVFILT_USER, EV_ADD | EV_ONESHOT, NOTE_TRIGGER, 0, nullptr);
        call(FN(kevent, kq, &event, 1, nullptr, 0, nullptr));
    }
}

void Async::submitIOBluetooth(uint64_t id, IOType ioType, CompletionResult& result) {
    addPending(id, btSockets, ioType, result);
}

bool Async::bluetoothComplete(uint64_t id, IOType ioType, IOReturn status) {
    auto pending = popPending(id, btSockets, ioType);
    if (!pending) return false;

    auto& result = **pending;

    // Resume caller
    result.error = status;
    result.coroHandle();
    return true;
}

void Async::bluetoothReadComplete(uint64_t id, const char* data, size_t dataLen) {
    btReads[id].emplace(std::in_place, data, dataLen);

    bluetoothComplete(id, IOType::Receive, kIOReturnSuccess);
}

void Async::bluetoothClosed(uint64_t id) {
    btReads[id].emplace();

    // Close events are determined by the receive result, resume the first read operation in the queue
    bluetoothComplete(id, IOType::Receive, kIOReturnSuccess);
}

std::optional<std::string> Async::getBluetoothReadResult(uint64_t id) {
    auto data = btReads[id].front();
    btReads[id].pop();
    return data;
}

void Async::clearBluetoothDataQueue(uint64_t id) {
    btReads.erase(id);
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
