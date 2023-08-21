// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_LINUX
#include <bit>
#include <memory>

#include <liburing.h>

#include "plat_linux/async_linux.hpp"
#include "sockets/delegates/connserver.hpp"
#include "sockets/incomingsocket.hpp"

template <auto Tag>
Task<SocketPtr> Delegates::ConnServer<Tag>::accept() {
    auto acceptResult = co_await Async::run([this](Async::CompletionResult& result) {
        auto addr = std::bit_cast<sockaddr*>(&_traits.addr);

        io_uring_sqe* sqe = Async::getUringSQE();
        io_uring_prep_accept(sqe, _handle, addr, &_traits.addrLen, 0);
        io_uring_sqe_set_data(sqe, &result);
        Async::submitRing();
    });

    Handle fd = acceptResult.res;
    co_return std::make_unique<IncomingSocket<Tag>>(fd);
}

template Task<SocketPtr> Delegates::ConnServer<SocketTag::IP>::accept();
template Task<SocketPtr> Delegates::ConnServer<SocketTag::BT>::accept();
#endif
