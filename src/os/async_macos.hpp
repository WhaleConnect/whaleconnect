// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_MACOS
#include <optional>
#include <string>

#include <IOKit/IOReturn.h>

#include "async.hpp"

#include "net/enums.hpp"

namespace Async {
    // The type of a Bluetooth I/O operation.
    enum class IOType { Send, Receive };

    // Makes a socket nonblocking for use with kqueue.
    void prepSocket(int s);

    // Submits an event to the kernel queue.
    void submitKqueue(int ident, IOType ioType, CompletionResult& result);

    // Cancels pending operations for a socket file descriptor.
    void cancelPending(int fd);

    // Creates a pending operation for a Bluetooth channel.
    void submitIOBluetooth(uint64_t id, IOType ioType, CompletionResult& result);

    // Signals completion of a Bluetooth operation.
    bool bluetoothComplete(uint64_t id, IOType ioType, IOReturn status);

    // Signals completion of a Bluetooth read operation.
    void bluetoothReadComplete(uint64_t id, const char* data, size_t dataLen);

    // Signals closure of a Bluetooth channel.
    void bluetoothClosed(uint64_t id);

    // Gets the first queued result of a Bluetooth read operation.
    std::optional<std::string> getBluetoothReadResult(uint64_t id);

    // Removes results from previous receive operations on a Bluetooth channel.
    void clearBluetoothDataQueue(uint64_t id);

    // Cancels all pending operations on a Bluetooth channel.
    void bluetoothCancel(uint64_t id);
}
#endif
