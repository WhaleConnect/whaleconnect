// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "sockets/delegates/closeable.hpp"
#include "sockets/enums.hpp"

template <>
void Delegates::Closeable<SocketTag::IP>::close() {
    shutdown(_handle, SHUT_RDWR);
    ::close(_handle);
}
#endif
