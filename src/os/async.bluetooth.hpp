// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// This header is only used on macOS.

#pragma once

#include <cstddef>

#include <IOKit/IOReturn.h>

#include "net/device.hpp"

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
