// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <exception>

#include <sys/event.h>
#include <sys/socket.h>

#include "client.hpp"

#include "net/netutils.hpp"
#include "os/async_macos.hpp"
#include "os/errcheck.hpp"

template <>
Task<> Delegates::Client<SocketTag::IP>::connect(Device device) {
    auto addr = NetUtils::resolveAddr(device);

    co_await NetUtils::loopWithAddr(addr.get(), [this](const AddrInfoType* result) -> Task<> {
        handle.reset(call(FN(socket, result->ai_family, result->ai_socktype, result->ai_protocol)));

        Async::prepSocket(*handle);

        // Start connect
        call(FN(::connect, *handle, result->ai_addr, result->ai_addrlen));
        co_await Async::run(std::bind_front(Async::submitKqueue, *handle, Async::IOType::Send));
    });
}
#endif
