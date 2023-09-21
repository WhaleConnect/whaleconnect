// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include <bit>

#include "sockethandle.hpp"

#include "net/enums.hpp"

template <auto Tag>
void Delegates::SocketHandle<Tag>::closeImpl() const {
    shutdown(handle, SD_BOTH);
    closesocket(handle);
}

template <auto Tag>
void Delegates::SocketHandle<Tag>::cancelIO() {
    CancelIoEx(std::bit_cast<HANDLE>(handle), nullptr);
}

template void Delegates::SocketHandle<SocketTag::IP>::closeImpl() const;
template void Delegates::SocketHandle<SocketTag::IP>::cancelIO();

template void Delegates::SocketHandle<SocketTag::BT>::closeImpl() const;
template void Delegates::SocketHandle<SocketTag::BT>::cancelIO();
#endif
