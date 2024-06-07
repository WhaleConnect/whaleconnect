// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bluetooth.hpp"

#include <queue>
#include <unordered_map>

using CompletionQueue = std::queue<Async::CompletionResult*>;

struct SocketQueue {
    CompletionQueue pendingReads;
    CompletionQueue pendingWrites;
};

using SocketQueueMap = std::unordered_map<swift::UInt, SocketQueue>;

// Queue for Bluetooth sockets
SocketQueueMap btSockets;

std::unordered_map<swift::UInt, std::queue<std::optional<std::string>>> btReads;
std::unordered_map<swift::UInt, std::queue<AsyncBT::BTAccept>> btAccepts;

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

void AsyncBT::submit(swift::UInt id, IOType ioType, Async::CompletionResult& result) {
    getPendingQueue(id, btSockets, ioType).push(&result);
}

std::optional<std::string> AsyncBT::getReadResult(swift::UInt id) {
    auto data = btReads[id].front();
    btReads[id].pop();
    return data;
}

AsyncBT::BTAccept AsyncBT::getAcceptResult(swift::UInt id) {
    auto client = btAccepts[id].front();
    btAccepts[id].pop();
    return client;
}

void AsyncBT::cancel(swift::UInt id) {
    // Loop through all pending events and send the "aborted" signal

    // clang-format doesn't recognize semicolons after loops
    // clang-format off
    while (bluetoothComplete(id, IOType::Send, kIOReturnAborted));
    while (bluetoothComplete(id, IOType::Receive, kIOReturnAborted));
    // clang-format on
}
