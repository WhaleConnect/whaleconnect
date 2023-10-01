// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include "cpp_bridge.hpp"

import os.async.platform;
import os.error;

// Wrappers for C++ async functions.
// The hash of the channel is passed to C++ code for identification of each channel.

void clearDataQueue(unsigned long channelHash) {
    Async::clearBluetoothDataQueue(channelHash);
}

void newData(unsigned long channelHash, const char* data, size_t dataLen) {
    Async::bluetoothReadComplete(channelHash, data, dataLen);
}

void outgoingComplete(unsigned long channelHash, IOReturn status) {
    Async::bluetoothComplete(channelHash, Async::IOType::Send, status);
}

void closed(unsigned long channelHash) {
    Async::bluetoothClosed(channelHash);
}
#endif
