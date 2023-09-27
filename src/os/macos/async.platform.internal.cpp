// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_MACOS
#include <cstdint>
#include <optional>

module os.async.platform.internal;
import os.async;
import os.async.platform;

// Gets the queue of pending operations for a socket/Bluetooth channel.
Async::Internal::CompletionQueue& getPendingQueue(uint64_t id, Async::Internal::SocketQueueMap& map,
                                                  Async::IOType ioType) {
    auto& queue = map[id];

    return (ioType == Async::IOType::Send) ? queue.pendingWrites : queue.pendingReads;
}

void Async::Internal::addPending(uint64_t id, SocketQueueMap& map, Async::IOType ioType,
                                 Async::CompletionResult& result) {
    getPendingQueue(id, map, ioType).push(&result);
}

Async::Internal::OptCompletionResult Async::Internal::popPending(uint64_t id, SocketQueueMap& map, IOType ioType) {
    auto& queue = getPendingQueue(id, map, ioType);
    if (queue.empty()) return std::nullopt;

    auto front = queue.front();
    queue.pop();
    return front;
}
#endif
