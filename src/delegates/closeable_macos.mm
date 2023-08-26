// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include "closeable.hpp"

#include "net/enums.hpp"
#include "os/async_macos.hpp"

template <>
void Delegates::Closeable<SocketTag::BT>::_closeImpl() const {
    [_handle close];
}

template <>
void Delegates::Closeable<SocketTag::BT>::cancelIO() {
    Async::bluetoothCancel([_handle channelHash]);
}
#endif
