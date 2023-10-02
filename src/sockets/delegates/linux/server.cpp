// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_LINUX
#include <bit>
#include <coroutine>
#include <functional>
#include <memory>

#include <liburing.h>
#include <netdb.h>
#include <netinet/in.h>

#include "os/check.hpp"

module sockets.delegates.server;
import net.netutils;
import os.async;
import os.async.platform;
import os.errcheck;
import sockets.incomingsocket;
import utils.task;

void startAccept(int s, sockaddr* clientAddr, socklen_t& clientLen, Async::CompletionResult& result) {
    io_uring_sqe* sqe = Async::getUringSQE();
    io_uring_prep_accept(sqe, s, clientAddr, &clientLen, 0);
    io_uring_sqe_set_data(sqe, &result);
    Async::submitRing();
}

template <>
ServerAddress Delegates::Server<SocketTag::IP>::startServer(const Device& serverInfo) {
    return NetUtils::startServer(serverInfo, handle);
}

template <>
Task<AcceptResult> Delegates::Server<SocketTag::IP>::accept() {
    sockaddr_storage client;
    auto clientAddr = std::bit_cast<sockaddr*>(&client);
    socklen_t clientLen = sizeof(client);

    auto acceptResult = co_await Async::run(std::bind_front(startAccept, *handle, clientAddr, clientLen));

    Device device = NetUtils::fromAddr(clientAddr, clientLen, ConnectionType::TCP);
    SocketHandle<SocketTag::IP> fd{ acceptResult.res };

    co_return { device, std::make_unique<IncomingSocket<SocketTag::IP>>(std::move(fd)) };
}

template <>
Task<DgramRecvResult> Delegates::Server<SocketTag::IP>::recvFrom(size_t) {
    // TODO
    co_return {};
}

template <>
Task<> Delegates::Server<SocketTag::IP>::sendTo(Device, std::string) {
    // TODO
    co_return;
}

template <>
Task<AcceptResult> Delegates::Server<SocketTag::BT>::accept() {
    // TODO
    co_return {};
}

template <>
ServerAddress Delegates::Server<SocketTag::BT>::startServer(const Device&) {
    // TODO
    return {};
}
#endif
