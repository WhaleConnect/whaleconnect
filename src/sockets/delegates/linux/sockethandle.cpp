// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <liburing.h>

module sockets.delegates.sockethandle;
import os.async.platform;
import net.enums;

template <auto Tag>
void Delegates::SocketHandle<Tag>::closeImpl() {
    io_uring_prep_shutdown(Async::getUringSQE(), handle, SHUT_RDWR);
    io_uring_prep_close(Async::getUringSQE(), handle);
    Async::submitRing();
}

template <auto Tag>
void Delegates::SocketHandle<Tag>::cancelIO() {
    Async::cancelPending(handle);
}

template void Delegates::SocketHandle<SocketTag::IP>::closeImpl();
template void Delegates::SocketHandle<SocketTag::IP>::cancelIO();

template void Delegates::SocketHandle<SocketTag::BT>::closeImpl();
template void Delegates::SocketHandle<SocketTag::BT>::cancelIO();
