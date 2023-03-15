// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>

#include "async_macos.hpp"

#include "os/errcheck.hpp"
#include "sockets/clientsocket.hpp"

template <>
Task<> ClientSocket<SocketTag::IP>::connect() const {
    // Make socket non-blocking
    int flags = call(FN(fcntl, _get(), F_GETFL, 0));
    call(FN(fcntl, _get(), F_SETFL, flags | O_NONBLOCK));

    // Start connect
    call(FN(::connect, _get(), _addr->ai_addr, _addr->ai_addrlen));
    co_await Async::run(std::bind_front(Async::submitKqueue, _get(), EVFILT_WRITE));
}
#endif
