// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module os.async.platform;
import external.platform;
import external.std;
import net.device;
import os.async;
import os.async.platform.internal;
import os.errcheck;
import os.error;

int currentKqueueIdx = 0;

// Queue for Bluetooth sockets
Async::Internal::SocketQueueMap btSockets;

std::unordered_map<Async::SwiftID, std::queue<std::optional<std::string>>> btReads;
std::unordered_map<Async::SwiftID, std::queue<Async::BTAccept>> btAccepts;

void Async::prepSocket(int s) {
    int flags = check(fcntl(s, F_GETFL, 0));
    check(fcntl(s, F_SETFL, flags | O_NONBLOCK));
}

void Async::submitKqueue(int ident, IOType ioType, CompletionResult& result) {
    // The EV_ONESHOT flag will delete the event once it is retrieved in one of the threads.
    // This ensures only one thread will wake up to handle the event.
    u32 ioTypeInt = std::to_underlying(ioType);

    // Pass I/O type as user data pointer
    auto typeData = reinterpret_cast<void*>(static_cast<u64>(ioTypeInt));
    i16 filt = ioType == IOType::Send ? EVFILT_WRITE : EVFILT_READ;

    std::array<struct kevent, 3> events{ {
        // Add and disable the I/O filter
        // If there's a problem with the file descriptor, kevent will exit early.
        { static_cast<unsigned long>(ident), filt, EV_ADD | EV_DISABLE, 0, 0, nullptr },

        // Request to add the operation to the pending queue
        // Early exiting of kevent will prevent dangling entries in the thread's queue if the socket's I/O filter didn't
        // make it into the kqueue.
        { Internal::ASYNC_ADD | ident, EVFILT_USER, EV_ADD | EV_ONESHOT, NOTE_TRIGGER | ioTypeInt, 0, &result },

        // Enable the I/O filter once the pending queue has been modified
        { static_cast<unsigned long>(ident), filt, EV_ENABLE | EV_ONESHOT, 0, 0, typeData },
    } };

    check(kevent(Internal::kqs[currentKqueueIdx], events.data(), events.size(), nullptr, 0, nullptr));

    // Cycle through all worker threads
    currentKqueueIdx = (currentKqueueIdx + 1) % Internal::kqs.size();
}

void Async::cancelPending(int fd) {
    for (auto kq : Internal::kqs) {
        // The file descriptor is used in "ident" so events can remain unique
        struct kevent event {
            Internal::ASYNC_CANCEL | fd, EVFILT_USER, EV_ADD | EV_ONESHOT, NOTE_TRIGGER, 0, nullptr
        };

        check(kevent(kq, &event, 1, nullptr, 0, nullptr));
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

void Async::bluetoothReadComplete(SwiftID id, const char* data, std::size_t dataLen) {
    btReads[id].emplace(std::in_place, data, dataLen);

    bluetoothComplete(id, IOType::Receive, kIOReturnSuccess);
}

void Async::bluetoothAcceptComplete(SwiftID id, const BluetoothMacOS::BTHandle& handle, const Device& device) {
    btAccepts[id].emplace(device, handle);

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

Async::BTAccept Async::getBluetoothAcceptResult(SwiftID id) {
    auto client = btAccepts[id].front();
    btAccepts[id].pop();
    return client;
}

void Async::clearBluetoothDataQueue(SwiftID id) {
    btReads.erase(id);
    btAccepts.erase(id);
}

void Async::bluetoothCancel(SwiftID id) {
    // Loop through all pending events and send the "aborted" signal

    // clang-format doesn't recognize semicolons after loops
    // clang-format off
    while (bluetoothComplete(id, IOType::Send, kIOReturnAborted));
    while (bluetoothComplete(id, IOType::Receive, kIOReturnAborted));
    // clang-format on
}
