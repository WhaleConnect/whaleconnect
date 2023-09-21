// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "sockethandle.hpp"

#include "net/enums.hpp"
#include "os/async_macos.hpp"

template <>
void Delegates::SocketHandle<SocketTag::IP>::closeImpl() const {
    shutdown(handle, SHUT_RDWR);
    ::close(handle);
}

template <>
void Delegates::SocketHandle<SocketTag::IP>::cancelIO() {
    Async::cancelPending(handle);
}
#endif
