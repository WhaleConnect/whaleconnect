// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <functional>

#include <netdb.h>
#include <netinet/in.h>

#include "server.hpp"

#include "delegates/delegates.hpp"
#include "net/enums.hpp"
#include "net/netutils.hpp"
#include "os/async_macos.hpp"
#include "os/errcheck.hpp"
#include "sockets/incomingsocket.hpp"

template <>
ServerAddress Delegates::Server<SocketTag::IP>::startServer(ConnectionType type, std::string_view addr, uint16_t port) {
    ServerAddress result = NetUtils::startServer(addr, port, _handle, type);

    Async::prepSocket(*_handle);
    return result;
}

template <>
Task<AcceptResult> Delegates::Server<SocketTag::IP>::accept() {
    co_await Async::run(std::bind_front(Async::submitKqueue, *_handle, Async::IOType::Receive));

    sockaddr_storage client;
    auto clientAddr = std::bit_cast<sockaddr*>(&client);
    socklen_t clientLen = sizeof(client);

    SocketHandle<SocketTag::IP> fd{ call(FN(::accept, *_handle, clientAddr, &clientLen)) };
    Device device = NetUtils::fromAddr(clientAddr, clientLen, ConnectionType::TCP);

    Async::prepSocket(*fd);
    co_return { device, std::make_unique<IncomingSocket<SocketTag::IP>>(std::move(fd)) };
}
#endif
