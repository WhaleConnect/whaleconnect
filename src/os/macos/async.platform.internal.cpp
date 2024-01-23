// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <optional>

module os.async.platform.internal;
import os.async;
import os.async.platform;

// Gets the queue of pending operations for a socket/Bluetooth channel.
Async::Internal::CompletionQueue& getPendingQueue(Async::SwiftID id, Async::Internal::SocketQueueMap& map,
    Async::IOType ioType) {
    auto& queue = map[id];

    return ioType == Async::IOType::Send ? queue.pendingWrites : queue.pendingReads;
}

void Async::Internal::addPending(SwiftID id, SocketQueueMap& map, Async::IOType ioType,
    Async::CompletionResult& result) {
    getPendingQueue(id, map, ioType).push(&result);
}

Async::Internal::OptCompletionResult Async::Internal::popPending(SwiftID id, SocketQueueMap& map, IOType ioType) {
    auto& queue = getPendingQueue(id, map, ioType);
    if (queue.empty()) return std::nullopt;

    auto front = queue.front();
    queue.pop();
    return front;
}
