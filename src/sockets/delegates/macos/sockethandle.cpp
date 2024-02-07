// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module sockets.delegates.sockethandle;
import external.platform;
import os.async.platform;
import net.enums;

template <>
void Delegates::SocketHandle<SocketTag::IP>::closeImpl() {
    shutdown(handle, SHUT_RDWR);
    ::close(handle);
}

template <>
void Delegates::SocketHandle<SocketTag::IP>::cancelIO() {
    Async::cancelPending(handle);
}

template <>
void Delegates::SocketHandle<SocketTag::BT>::closeImpl() {
    handle->close();
}

template <>
void Delegates::SocketHandle<SocketTag::BT>::cancelIO() {
    Async::bluetoothCancel(handle->getHash());
}
