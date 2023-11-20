// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_LINUX
#include <bit>
#include <coroutine> // IWYU pragma: keep
#include <functional>
#include <memory>

#include <liburing.h>
#include <netdb.h>
#include <netinet/in.h>

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
Task<DgramRecvResult> Delegates::Server<SocketTag::IP>::recvFrom(size_t size) {
    // io_uring currently does not support recvfrom so recvmsg must be used instead:
    // https://github.com/axboe/liburing/issues/397
    // https://github.com/axboe/liburing/discussions/581

    sockaddr_storage from;
    auto fromAddr = std::bit_cast<sockaddr*>(&from);
    socklen_t len = sizeof(from);
    std::string data(size, 0);

    iovec iov{
        .iov_base = data.data(),
        .iov_len = data.size(),
    };

    msghdr msg{
        .msg_name = &from,
        .msg_namelen = len,
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = nullptr,
        .msg_controllen = 0,
        .msg_flags = 0,
    };

    auto recvResult = co_await Async::run([this, &msg](Async::CompletionResult& result) {
        io_uring_sqe* sqe = Async::getUringSQE();
        io_uring_prep_recvmsg(sqe, *handle, &msg, MSG_NOSIGNAL);
        io_uring_sqe_set_data(sqe, &result);
        Async::submitRing();
    });

    data.resize(recvResult.res);
    co_return { NetUtils::fromAddr(fromAddr, len, ConnectionType::UDP), data };
}

template <>
Task<> Delegates::Server<SocketTag::IP>::sendTo(Device device, std::string data) {
    auto addr = NetUtils::resolveAddr(device, false);

    co_await NetUtils::loopWithAddr(addr.get(), [this, &data](const AddrInfoType* resolveRes) -> Task<> {
        co_await Async::run([this, &data, &resolveRes](Async::CompletionResult& result) {
            io_uring_sqe* sqe = Async::getUringSQE();
            io_uring_prep_sendto(sqe, *handle, data.data(), data.size(), MSG_NOSIGNAL, resolveRes->ai_addr,
                resolveRes->ai_addrlen);
            io_uring_sqe_set_data(sqe, &result);
            Async::submitRing();
        });
    });
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
