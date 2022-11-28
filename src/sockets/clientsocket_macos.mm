// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_APPLE
#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>

#include "clientsocket.hpp"
#include "os/async.hpp"
#include "os/errcheck.hpp"

template <>
Task<> ClientSocket<SocketTag::IP>::connect() const {
    // Make socket non-blocking
    int flags = EXPECT_NONERROR(fcntl, _handle, F_GETFL, 0);
    EXPECT_NONERROR(fcntl, _handle, F_SETFL, flags | O_NONBLOCK);

    // Start connect
    EXPECT_ZERO(::connect, _handle, _addr->ai_addr, _addr->ai_addrlen);
    co_await Async::run(std::bind_front(Async::submitKqueue, _handle, EVFILT_WRITE));
}

template <>
std::unique_ptr<ClientSocket<SocketTag::BT>> createClientSocket(const Device& device) {
    return std::make_unique<ClientSocket<SocketTag::BT>>(SocketTraits<SocketTag::BT>::invalidHandle, device);
}

template <>
Task<> ClientSocket<SocketTag::BT>::connect() const {
    co_return;
}
#endif
