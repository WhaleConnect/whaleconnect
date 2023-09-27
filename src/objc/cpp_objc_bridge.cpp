// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include "cpp_objc_bridge.hpp"

import os.async.platform;
import os.error;

[[noreturn]] void CppObjCBridge::throwSystemError(IOReturn res, std::string_view fnName) {
    throw System::SystemError{ res, System::ErrorType::IOReturn, fnName };
}

// Objective-C++ wrappers for C++ async functions.
// The hash of the channel is passed to C++ code for identification of each channel.

void CppObjCBridge::Async::clearDataQueue(IOBluetoothObject* channel) {
    ::Async::clearBluetoothDataQueue(getChannelHash(channel));
}

void CppObjCBridge::Async::newData(IOBluetoothObject* channel, const char* data, size_t dataLen) {
    ::Async::bluetoothReadComplete(getChannelHash(channel), data, dataLen);
}

void CppObjCBridge::Async::outgoingComplete(IOBluetoothObject* channel, IOReturn status) {
    ::Async::bluetoothComplete(getChannelHash(channel), ::Async::IOType::Send, status);
}

void CppObjCBridge::Async::closed(IOBluetoothObject* channel) {
    ::Async::bluetoothClosed(getChannelHash(channel));
}
#endif
