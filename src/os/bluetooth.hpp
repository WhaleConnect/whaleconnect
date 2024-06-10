// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// This header is only used on macOS.

#pragma once

#include <cstddef>

#include <IOKit/IOReturn.h>

#include "net/device.hpp"

#ifndef NO_EXPOSE_INTERNAL
#include <swiftToCxx/_SwiftCxxInteroperability.h>

#include "async.hpp"
#endif

// The type of a Bluetooth I/O operation.
enum class IOType { Send, Receive };

// Removes results from previous receive/accept operations on a Bluetooth channel.
void clearBluetoothDataQueue(unsigned long id);

// Signals completion of a Bluetooth read operation.
void bluetoothReadComplete(unsigned long id, const char* data, std::size_t dataLen);

// Signals completion of a Bluetooth operation.
bool bluetoothComplete(unsigned long id, IOType ioType, IOReturn status);

// Signals completion of a Bluetooth accept operation.
void bluetoothAcceptComplete(unsigned long id, const void* handle, const Device& device);

// Signals closure of a Bluetooth channel.
void bluetoothClosed(unsigned long id);

// Definitions not exposed to Swift.
#ifndef NO_EXPOSE_INTERNAL
namespace AsyncBT {
    struct BTAccept {
        Device from;
        BluetoothMacOS::BTHandle handle;

        // TODO: Remove when Apple Clang supports P0960
        BTAccept(const Device& from, BluetoothMacOS::BTHandle handle) : from(from), handle(handle) {}
    };

    // Creates a pending operation for a Bluetooth channel.
    void submit(swift::UInt id, IOType ioType, Async::CompletionResult& result);

    // Gets the first queued result of a Bluetooth read operation.
    std::optional<std::string> getReadResult(swift::UInt id);

    // Gets the first queued result of a Bluetooth accept operation.
    BTAccept getAcceptResult(swift::UInt id);

    // Cancels all pending operations on a Bluetooth channel.
    void cancel(swift::UInt id);
}
#endif
