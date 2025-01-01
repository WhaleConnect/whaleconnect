// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sockets/delegates/sockethandle.hpp"

#include "net/enums.hpp"
#include "os/async.hpp"

template <auto Tag>
void Delegates::SocketHandle<Tag>::closeImpl() {
    Async::submit(Async::Shutdown{ { **this, nullptr } });
    Async::submit(Async::Close{ { **this, nullptr } });
}

template <auto Tag>
void Delegates::SocketHandle<Tag>::cancelIO() {
    Async::submit(Async::Cancel{ { **this, nullptr } });
}

template void Delegates::SocketHandle<SocketTag::IP>::closeImpl();
template void Delegates::SocketHandle<SocketTag::IP>::cancelIO();

template void Delegates::SocketHandle<SocketTag::BT>::closeImpl();
template void Delegates::SocketHandle<SocketTag::BT>::cancelIO();
