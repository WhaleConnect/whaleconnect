// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include "sockethandle.hpp"

#include "net/enums.hpp"
#include "os/async_macos.hpp"

template <>
void Delegates::SocketHandle<SocketTag::BT>::closeImpl() const {
    [handle close];
}

template <>
void Delegates::SocketHandle<SocketTag::BT>::cancelIO() {
    Async::bluetoothCancel([handle channelHash]);
}
#endif
