// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>

#include "os/errcheck.hpp"
#include "plat_macos/async_macos.hpp"
#include "sockets/delegates/client.hpp"

template <>
Task<> Delegates::Client<SocketTag::IP>::connect() const {
    // Make socket non-blocking
    int flags = call(FN(fcntl, _handle, F_GETFL, 0));
    call(FN(fcntl, _handle, F_SETFL, flags | O_NONBLOCK));

    // Start connect
    call(FN(::connect, _handle, _traits.addr->ai_addr, _traits.addr->ai_addrlen));
    co_await Async::run(std::bind_front(Async::submitKqueue, _handle, Async::IOType::Send));
}
#endif
