// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_LINUX
#include <liburing.h>

module sockets.delegates.sockethandle;
import os.async.platform;
import net.enums;

template <auto Tag>
void Delegates::SocketHandle<Tag>::closeImpl() const {
    io_uring_prep_shutdown(Async::getUringSQE(), handle, SHUT_RDWR);
    io_uring_prep_close(Async::getUringSQE(), handle);
    Async::submitRing();
}

template <auto Tag>
void Delegates::SocketHandle<Tag>::cancelIO() {
    Async::cancelPending(handle);
}

template void Delegates::SocketHandle<SocketTag::IP>::closeImpl() const;
template void Delegates::SocketHandle<SocketTag::IP>::cancelIO();

template void Delegates::SocketHandle<SocketTag::BT>::closeImpl() const;
template void Delegates::SocketHandle<SocketTag::BT>::cancelIO();
#endif
