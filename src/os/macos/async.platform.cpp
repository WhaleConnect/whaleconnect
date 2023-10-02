// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_MACOS
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include <IOKit/IOReturn.h>
#include <magic_enum.hpp>
#include <sys/event.h>
#include <sys/fcntl.h>

#include "os/check.hpp"

module os.async.platform;
import os.async;
import os.async.platform.internal;
import os.errcheck;
import os.error;

int currentKqueueIdx = 0;

// Queue for Bluetooth sockets
Async::Internal::SocketQueueMap btSockets;

// Bluetooth read results
std::unordered_map<Async::SwiftID, std::queue<std::optional<std::string>>> btReads;

void Async::prepSocket(int s) {
    int flags = CHECK(fcntl(s, F_GETFL, 0));
    CHECK(fcntl(s, F_SETFL, flags | O_NONBLOCK));
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
    EV_SET(&events[1], Internal::ASYNC_ADD | ident, EVFILT_USER, EV_ADD | EV_ONESHOT, NOTE_TRIGGER | ioTypeInt, 0,
           &result);

    // Enable the I/O filter once the pending queue has been modified
    EV_SET(&events[2], ident, filt, EV_ENABLE | EV_ONESHOT, 0, 0, typeData);

    CHECK(kevent(Internal::kqs[currentKqueueIdx], events.data(), events.size(), nullptr, 0, nullptr));

    // Cycle through all worker threads
    currentKqueueIdx = (currentKqueueIdx + 1) % Internal::kqs.size();
}

void Async::cancelPending(int fd) {
    for (auto kq : Internal::kqs) {
        struct kevent event {};

        // The file descriptor is used in "ident" so events can remain unique
        EV_SET(&event, Internal::ASYNC_CANCEL | fd, EVFILT_USER, EV_ADD | EV_ONESHOT, NOTE_TRIGGER, 0, nullptr);
        CHECK(kevent(kq, &event, 1, nullptr, 0, nullptr));
    }
}

void Async::submitIOBluetooth(SwiftID id, IOType ioType, CompletionResult& result) {
    Internal::addPending(id, btSockets, ioType, result);
}

bool Async::bluetoothComplete(SwiftID id, IOType ioType, IOReturn status) {
    auto pending = Internal::popPending(id, btSockets, ioType);
    if (!pending) return false;

    auto& result = **pending;

    // Resume caller
    result.error = status;
    result.coroHandle();
    return true;
}

void Async::bluetoothReadComplete(SwiftID id, const char* data, size_t dataLen) {
    btReads[id].emplace(std::in_place, data, dataLen);

    bluetoothComplete(id, IOType::Receive, kIOReturnSuccess);
}

void Async::bluetoothClosed(SwiftID id) {
    btReads[id].emplace();

    // Close events are determined by the receive result, resume the first read operation in the queue
    bluetoothComplete(id, IOType::Receive, kIOReturnSuccess);
}

std::optional<std::string> Async::getBluetoothReadResult(SwiftID id) {
    auto data = btReads[id].front();
    btReads[id].pop();
    return data;
}

void Async::clearBluetoothDataQueue(SwiftID id) {
    btReads.erase(id);
}

void Async::bluetoothCancel(SwiftID id) {
    // Loop through all pending events and send the "aborted" signal

    // clang-format doesn't recognize semicolons after loops
    // clang-format off
    while (bluetoothComplete(id, IOType::Send, kIOReturnAborted));
    while (bluetoothComplete(id, IOType::Receive, kIOReturnAborted));
    // clang-format on
}
#endif
