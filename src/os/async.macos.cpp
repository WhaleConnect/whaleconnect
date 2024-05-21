// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "async.hpp"

#include <cerrno>
#include <coroutine>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <variant>

#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "async.bluetooth.hpp"
#include "errcheck.hpp"
#include "utils/overload.hpp"

//   | A kevent is identified by the (ident, filter) pair; there may only be one unique kevent per kqueue.
// https://man.freebsd.org/cgi/man.cgi?kqueue
//
// There are two possible filters on sockets: EVFILT_READ and EVFILT_WRITE
// This will be reduced into a single bit (read == 0; write == 1) and combined with the socket file descriptor to
// produce a unique uint64_t for the pair.
constexpr auto filtWrite = 1UL << 33;

using CompletionQueue = std::queue<Async::CompletionResult*>;
using OptCompletionResult = std::optional<Async::CompletionResult*>;

struct SocketQueue {
    CompletionQueue pendingReads;
    CompletionQueue pendingWrites;
};

using SocketQueueMap = std::unordered_map<swift::UInt, SocketQueue>;

// Queue for Bluetooth sockets
SocketQueueMap btSockets;

std::unordered_map<swift::UInt, std::queue<std::optional<std::string>>> btReads;
std::unordered_map<swift::UInt, std::queue<Async::BTAccept>> btAccepts;

std::uint64_t getMapID(int s, std::int16_t filt) {
    std::uint64_t filterBit = filt == EVFILT_WRITE ? filtWrite : 0;
    return static_cast<std::uint64_t>(s) | filterBit;
}

// Gets the queue of pending operations for a socket/Bluetooth channel.
CompletionQueue& getPendingQueue(swift::UInt id, SocketQueueMap& map, IOType ioType) {
    auto& queue = map[id];

    return ioType == IOType::Send ? queue.pendingWrites : queue.pendingReads;
}

bool bluetoothComplete(unsigned long id, IOType ioType, IOReturn status) {
    auto& queue = getPendingQueue(id, btSockets, ioType);
    if (queue.empty()) return false;

    auto pending = queue.front();
    queue.pop();

    auto& result = *pending;

    // Resume caller
    result.error = status;
    result.coroHandle();
    return true;
}

void bluetoothReadComplete(unsigned long id, const char* data, std::size_t dataLen) {
    btReads[id].emplace(std::in_place, data, dataLen);

    bluetoothComplete(id, IOType::Receive, kIOReturnSuccess);
}

void bluetoothAcceptComplete(unsigned long id, const void* handle, const Device& device) {
    btAccepts[id].emplace(device, *static_cast<const BluetoothMacOS::BTHandle*>(handle));

    bluetoothComplete(id, IOType::Receive, kIOReturnSuccess);
}

void bluetoothClosed(unsigned long id) {
    btReads[id].emplace();

    // Close events are determined by the receive result, resume the first read operation in the queue
    bluetoothComplete(id, IOType::Receive, kIOReturnSuccess);
}

void clearBluetoothDataQueue(unsigned long id) {
    btReads.erase(id);
    btAccepts.erase(id);
}

void Async::prepSocket(int s) {
    int flags = check(fcntl(s, F_GETFL, 0));
    check(fcntl(s, F_SETFL, flags | O_NONBLOCK));
}

Async::WorkerThread::WorkerThread(unsigned int, unsigned int) :
    kq(check(kqueue())), thread(&Async::WorkerThread::eventLoop, this) {}

Async::WorkerThread::~WorkerThread() {
    stopWait(asyncInterrupt);
    signalPending();

    thread.join();
    close(kq);
}

void Async::WorkerThread::stopWait(std::uintptr_t key) {
    struct kevent event {
        key, EVFILT_USER, EV_ADD | EV_ONESHOT, NOTE_TRIGGER, 0, nullptr
    };

    kevent(kq, &event, 1, nullptr, 0, nullptr);
    numOperations.fetch_add(1, std::memory_order_relaxed);
}

void Async::WorkerThread::cancel(std::uint64_t ident) {
    std::scoped_lock lock{ pendingMtx };
    for (std::int16_t filt : { EVFILT_READ, EVFILT_WRITE }) {
        std::uint64_t mapID = getMapID(ident, filt);

        if (pendingEvents.contains(mapID)) {
            struct kevent event {
                ident, filt, EV_DELETE, 0, 0, nullptr
            };

            kevent(kq, &event, 1, nullptr, 0, nullptr);

            auto& result = *pendingEvents[mapID];
            pendingEvents.erase(mapID);
            result.error = ECANCELED;
            result.coroHandle();
        }
    }
}

void Async::WorkerThread::handleOperation(std::vector<struct kevent>& events, const Operation& next) {
    auto submit = [&](int id, std::int16_t filt, Async::CompletionResult* result) {
        const std::uint16_t flags = EV_ADD | EV_ONESHOT | EV_RECEIPT;
        events.push_back({ static_cast<unsigned long>(id), filt, flags, 0, 0, result });

        std::scoped_lock lock{ pendingMtx };
        pendingEvents[getMapID(id, filt)] = result;
    };

    Overload visitor{
        [&](const Async::Connect& op) { submit(op.handle, EVFILT_WRITE, op.result); },
        [&](const Async::Accept& op) { submit(op.handle, EVFILT_READ, op.result); },
        [&](const Async::Send& op) { submit(op.handle, EVFILT_WRITE, op.result); },
        [&](const Async::SendTo& op) { submit(op.handle, EVFILT_WRITE, op.result); },
        [&](const Async::Receive& op) { submit(op.handle, EVFILT_READ, op.result); },
        [&](const Async::ReceiveFrom& op) { submit(op.handle, EVFILT_READ, op.result); },
        [&](const Async::Shutdown& op) { shutdown(op.handle, SHUT_RDWR); },
        [&](const Async::Close& op) { close(op.handle); },
        [&](const Async::Cancel& op) { cancel(op.handle); },
    };

    std::visit(visitor, next);
}

std::vector<struct kevent> Async::WorkerThread::processOperations() {
    std::scoped_lock lock{ operationsMtx };

    std::vector<struct kevent> events(operations.size());
    numOperations.fetch_add(operations.size(), std::memory_order_relaxed);
    while (!operations.empty()) {
        auto next = operations.front();
        operations.pop();
        handleOperation(events, next);
    }

    return events;
}

bool Async::WorkerThread::submit() {
    auto events = processOperations();
    if (kevent(kq, events.data(), events.size(), events.data(), events.size(), nullptr) == 0) return true;

    for (const auto& i : events) {
        if (i.flags & EV_ERROR && i.data != 0 && i.udata) {
            deletePending(i);

            auto& result = *static_cast<CompletionResult*>(i.udata);
            result.error = i.data;
            result.coroHandle();
        }
    }

    return false;
}

void Async::WorkerThread::deletePending(const struct kevent& event) {
    std::scoped_lock lock{ pendingMtx };
    pendingEvents.erase(getMapID(event.ident, event.filter));
}

void Async::WorkerThread::eventLoop() {
    while (true) {
        bool expected = true;
        if (operationsPending.compare_exchange_weak(expected, false, std::memory_order_relaxed)) {
            if (!submit()) continue;
        } else if (numOperations.load(std::memory_order_relaxed) == 0) {
            operationsPending.wait(false, std::memory_order_relaxed);
            continue;
        }

        struct kevent event {};

        if (kevent(kq, nullptr, 0, &event, 1, nullptr) == -1) continue;
        numOperations.fetch_sub(1, std::memory_order_relaxed);

        // Handle user events
        if (event.filter == EVFILT_USER) {
            if (event.ident == asyncInterrupt) break;

            continue;
        }

        // Pop an event from the map and get its completion result
        auto& result = *static_cast<CompletionResult*>(event.udata);

        // Set error and result
        if (event.flags & EV_EOF) result.error = static_cast<System::ErrorCode>(event.fflags);
        else result.res = static_cast<int>(event.data);

        deletePending(event);
        result.coroHandle();
    }
}

void Async::submitIOBluetooth(swift::UInt id, IOType ioType, CompletionResult& result) {
    getPendingQueue(id, btSockets, ioType).push(&result);
}

std::optional<std::string> Async::getBluetoothReadResult(swift::UInt id) {
    auto data = btReads[id].front();
    btReads[id].pop();
    return data;
}

Async::BTAccept Async::getBluetoothAcceptResult(swift::UInt id) {
    auto client = btAccepts[id].front();
    btAccepts[id].pop();
    return client;
}

void Async::bluetoothCancel(swift::UInt id) {
    // Loop through all pending events and send the "aborted" signal

    // clang-format doesn't recognize semicolons after loops
    // clang-format off
    while (bluetoothComplete(id, IOType::Send, kIOReturnAborted));
    while (bluetoothComplete(id, IOType::Receive, kIOReturnAborted));
    // clang-format on
}
