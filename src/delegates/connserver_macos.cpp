// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <functional>

#include <netdb.h>
#include <netinet/in.h>

#include "connserver.hpp"

#include "delegates/delegates.hpp"
#include "net/enums.hpp"
#include "net/netutils.hpp"
#include "os/async_macos.hpp"
#include "os/errcheck.hpp"
#include "sockets/incomingsocket.hpp"

template <>
Task<AcceptResult> Delegates::ConnServer<SocketTag::IP>::accept() {
    co_await Async::run(std::bind_front(Async::submitKqueue, *_handle, Async::IOType::Receive));

    sockaddr_in6 client;
    auto clientAddr = std::bit_cast<sockaddr*>(&client);
    unsigned int clientLen = sizeof(client);

    Device device = NetUtils::fromAddr(clientAddr, clientLen, ConnectionType::TCP);
    SocketHandle<SocketTag::IP> fd{ call(FN(::accept, *_handle, clientAddr, &clientLen)) };

    co_return { device, std::make_unique<IncomingSocket<SocketTag::IP>>(std::move(fd)) };
}
#endif
