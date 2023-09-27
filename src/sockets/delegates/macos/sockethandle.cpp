// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_MACOS
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "objc/cpp_objc_bridge.hpp"

module sockets.delegates.sockethandle;
import os.async.platform;
import net.enums;

template <>
void Delegates::SocketHandle<SocketTag::IP>::closeImpl() const {
    shutdown(handle, SHUT_RDWR);
    ::close(handle);
}

template <>
void Delegates::SocketHandle<SocketTag::IP>::cancelIO() {
    Async::cancelPending(handle);
}

template <>
void Delegates::SocketHandle<SocketTag::BT>::closeImpl() const {
    CppObjCBridge::Bluetooth::close(handle);
}

template <>
void Delegates::SocketHandle<SocketTag::BT>::cancelIO() {
    Async::bluetoothCancel(CppObjCBridge::getBTHandleHash(handle));
}
#endif
