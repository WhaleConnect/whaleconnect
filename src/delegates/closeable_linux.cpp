// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_LINUX
#include <liburing.h>

#include "closeable.hpp"

#include "os/async_linux.hpp"
#include "os/errcheck.hpp"

template <auto Tag>
void Delegates::Closeable<Tag>::_closeImpl() const {
    io_uring_prep_shutdown(Async::getUringSQE(), _handle, SHUT_RDWR);
    io_uring_prep_close(Async::getUringSQE(), _handle);
    Async::submitRing();
}

template <auto Tag>
void Delegates::Closeable<Tag>::cancelIO() {
    Async::cancelPending(_handle);
}

template void Delegates::Closeable<SocketTag::IP>::_closeImpl() const;
template void Delegates::Closeable<SocketTag::IP>::cancelIO();

template void Delegates::Closeable<SocketTag::BT>::_closeImpl() const;
template void Delegates::Closeable<SocketTag::BT>::cancelIO();
#endif
