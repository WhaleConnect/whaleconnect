// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_MACOS
#include <IOKit/IOReturn.h>

#include "os/async.hpp"

namespace Async {
    // The type of a Bluetooth I/O operation.
    enum class BluetoothIOType { Send, Receive };

    // Submits an event to the kernel queue.
    void submitKqueue(int ident, int filter, CompletionResult& result);

    // Cancels pending operations for a socket file descriptor.
    void cancelPending(int fd);

    void submitIOBluetooth(uint64_t id, BluetoothIOType type, CompletionResult& result);

    void bluetoothComplete(uint64_t id, BluetoothIOType type, IOReturn status);

    void bluetoothReadComplete(uint64_t id, const char* data, size_t dataLen);

    std::string getBluetoothReadResult(uint64_t id);
}
#endif
