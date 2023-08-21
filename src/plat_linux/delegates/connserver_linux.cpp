// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sockets/enums.hpp"
#if OS_LINUX
#include <bit>
#include <memory>

#include <liburing.h>
#include <netdb.h>
#include <netinet/in.h>

#include "os/errcheck.hpp"
#include "plat_linux/async_linux.hpp"
#include "sockets/delegates/connserver.hpp"
#include "sockets/incomingsocket.hpp"

template <auto Tag>
Task<AcceptResult> Delegates::ConnServer<Tag>::accept() {
    sockaddr_in6 client;
    auto clientAddr = std::bit_cast<sockaddr*>(&client);
    unsigned int clientLen = sizeof(client);

    auto acceptResult = co_await Async::run([this, &clientAddr, &clientLen](Async::CompletionResult& result) {
        io_uring_sqe* sqe = Async::getUringSQE();
        io_uring_prep_accept(sqe, _handle, clientAddr, &clientLen, 0);
        io_uring_sqe_set_data(sqe, &result);
        Async::submitRing();
    });

    std::string clientIP(INET6_ADDRSTRLEN, '\0');
    call(FN(getnameinfo, clientAddr, clientLen, clientIP.data(), clientIP.size(), nullptr, 0, 0), checkZero,
         useReturnCode, System::ErrorType::AddrInfo);

    Device device{ ConnectionType::TCP, "", clientIP, client.sin6_port };
    Handle fd = acceptResult.res;

    co_return { device, std::make_unique<IncomingSocket<Tag>>(fd) };
}

template Task<AcceptResult> Delegates::ConnServer<SocketTag::IP>::accept();
template Task<AcceptResult> Delegates::ConnServer<SocketTag::BT>::accept();
#endif
