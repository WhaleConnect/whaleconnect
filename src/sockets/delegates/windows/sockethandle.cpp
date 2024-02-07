// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module sockets.delegates.sockethandle;
import external.platform;
import net.enums;

template <auto Tag>
void Delegates::SocketHandle<Tag>::closeImpl() {
    shutdown(handle, SD_BOTH);
    closesocket(handle);
}

template <auto Tag>
void Delegates::SocketHandle<Tag>::cancelIO() {
    CancelIoEx(reinterpret_cast<HANDLE>(handle), nullptr);
}

template void Delegates::SocketHandle<SocketTag::IP>::closeImpl();
template void Delegates::SocketHandle<SocketTag::IP>::cancelIO();

template void Delegates::SocketHandle<SocketTag::BT>::closeImpl();
template void Delegates::SocketHandle<SocketTag::BT>::cancelIO();
