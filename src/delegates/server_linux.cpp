// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_LINUX
#include <bit>
#include <memory>

#include <liburing.h>
#include <netdb.h>
#include <netinet/in.h>

#include "server.hpp"

#include "net/enums.hpp"
#include "net/netutils.hpp"
#include "os/async_linux.hpp"
#include "os/errcheck.hpp"
#include "sockets/incomingsocket.hpp"

template <auto Tag>
Task<AcceptResult> Delegates::Server<Tag>::accept() {
    sockaddr_in6 client;
    auto clientAddr = std::bit_cast<sockaddr*>(&client);
    unsigned int clientLen = sizeof(client);

    auto acceptResult = co_await Async::run([this, &clientAddr, &clientLen](Async::CompletionResult& result) {
        io_uring_sqe* sqe = Async::getUringSQE();
        io_uring_prep_accept(sqe, *_handle, clientAddr, &clientLen, 0);
        io_uring_sqe_set_data(sqe, &result);
        Async::submitRing();
    });

    Device device = NetUtils::fromAddr(clientAddr, clientLen, ConnectionType::TCP);
    SocketHandle<Tag> fd{ acceptResult.res };

    co_return { device, std::make_unique<IncomingSocket<Tag>>(std::move(fd)) };
}

template Task<AcceptResult> Delegates::Server<SocketTag::IP>::accept();
template Task<AcceptResult> Delegates::Server<SocketTag::BT>::accept();
#endif
