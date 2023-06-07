// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_MACOS
#include <optional>
#include <string>

#include <IOKit/IOReturn.h>

#include "os/async.hpp"

namespace Async {
    // The type of a Bluetooth I/O operation.
    enum class BluetoothIOType { Send, Receive };

    // Submits an event to the kernel queue.
    void submitKqueue(int ident, int filter, CompletionResult& result);

    // Cancels pending operations for a socket file descriptor.
    void cancelPending(int fd);

    // Creates a pending operation for a Bluetooth channel.
    void submitIOBluetooth(uint64_t id, BluetoothIOType type, CompletionResult& result);

    // Signals completion of a Bluetooth operation.
    void bluetoothComplete(uint64_t id, BluetoothIOType type, IOReturn status);

    // Signals completion of a Bluetooth read operation.
    void bluetoothReadComplete(uint64_t id, const char* data, size_t dataLen);

    // Signals closure of a Bluetooth channel.
    void bluetoothClosed(uint64_t id);

    // Gets the first queued result of a Bluetooth read operation.
    std::optional<std::string> getBluetoothReadResult(uint64_t id);
}
#endif
