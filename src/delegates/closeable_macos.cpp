// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "closeable.hpp"

#include "net/enums.hpp"
#include "os/async_macos.hpp"

template <>
void Delegates::Closeable<SocketTag::IP>::_closeImpl() const {
    shutdown(_handle, SHUT_RDWR);
    ::close(_handle);
}

template <>
void Delegates::Closeable<SocketTag::IP>::cancelIO() {
    Async::cancelPending(_handle);
}
#endif
