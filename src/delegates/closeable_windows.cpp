// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include "closeable.hpp"

#include "sockets/enums.hpp"

template <auto Tag>
void Delegates::Closeable<Tag>::_closeImpl() const {
    shutdown(_handle, SD_BOTH);
    closesocket(_handle);
}

template <auto Tag>
void Delegates::Cancel<Tag>::cancelIO() {
    CancelIoEx(std::bit_cast<HANDLE>(_handle), nullptr);
}

template void Delegates::Closeable<SocketTag::IP>::_closeImpl() const;
template void Delegates::Closeable<SocketTag::IP>::cancelIO();

template void Delegates::Closeable<SocketTag::BT>::_closeImpl() const;
template void Delegates::Closeable<SocketTag::BT>::cancelIO();
#endif
