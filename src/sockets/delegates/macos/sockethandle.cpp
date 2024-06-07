// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sockets/delegates/sockethandle.hpp"

#include "net/enums.hpp"
#include "os/async.hpp"
#include "os/bluetooth.hpp"

template <>
void Delegates::SocketHandle<SocketTag::IP>::closeImpl() {
    Async::submit(Async::Shutdown{ { **this, nullptr } });
    Async::submit(Async::Close{ { **this, nullptr } });
}

template <>
void Delegates::SocketHandle<SocketTag::IP>::cancelIO() {
    Async::submit(Async::Cancel{ { **this, nullptr } });
}

template <>
void Delegates::SocketHandle<SocketTag::BT>::closeImpl() {
    handle->close();
}

template <>
void Delegates::SocketHandle<SocketTag::BT>::cancelIO() {
    AsyncBT::cancel(handle->getHash());
}
