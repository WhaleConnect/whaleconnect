// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <exception>

#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>

#include "client.hpp"

#include "net/netutils.hpp"
#include "os/async_macos.hpp"
#include "os/errcheck.hpp"

template <>
Task<> Delegates::Client<SocketTag::IP>::connect() {
    auto addr = NetUtils::resolveAddr(_device, IPType::None);

    co_await NetUtils::loopWithAddr(addr.get(), [this](const AddrInfoType* result) -> Task<> {
        _handle.reset(call(FN(socket, result->ai_family, result->ai_socktype, result->ai_protocol)));

        // Make socket non-blocking
        int flags = call(FN(fcntl, *_handle, F_GETFL, 0));
        call(FN(fcntl, *_handle, F_SETFL, flags | O_NONBLOCK));

        // Start connect
        call(FN(::connect, *_handle, result->ai_addr, result->ai_addrlen));
        co_await Async::run(std::bind_front(Async::submitKqueue, *_handle, Async::IOType::Send));
    });
}
#endif
