// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_LINUX
#include <bit>
#include <memory>

#include <liburing.h>
#include <netinet/in.h>

#include "plat_linux/async_linux.hpp"
#include "sockets/delegates/connserver.hpp"
#include "sockets/incomingsocket.hpp"

template <auto Tag>
Task<SocketPtr> Delegates::ConnServer<Tag>::accept() {
    auto acceptResult = co_await Async::run([this](Async::CompletionResult& result) {
        sockaddr_in6 client;
        unsigned int clientLen = sizeof(client);

        io_uring_sqe* sqe = Async::getUringSQE();
        io_uring_prep_accept(sqe, _handle, std::bit_cast<sockaddr*>(&client), &clientLen, 0);
        io_uring_sqe_set_data(sqe, &result);
        Async::submitRing();
    });

    Handle fd = acceptResult.res;
    co_return std::make_unique<IncomingSocket<Tag>>(fd);
}

template Task<SocketPtr> Delegates::ConnServer<SocketTag::IP>::accept();
template Task<SocketPtr> Delegates::ConnServer<SocketTag::BT>::accept();
#endif
