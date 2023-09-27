// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_LINUX
#include <coroutine>
#include <optional>

#include <liburing.h>

#include "os/fn.hpp"

module sockets.delegates.bidirectional;
import net.enums;
import os.async;
import os.async.platform;
import os.errcheck;
import utils.task;

template <auto Tag>
Task<> Delegates::Bidirectional<Tag>::send(std::string data) {
    co_await Async::run([this, &data](Async::CompletionResult& result) {
        io_uring_sqe* sqe = Async::getUringSQE();
        io_uring_prep_send(sqe, *handle, data.data(), data.size(), MSG_NOSIGNAL);
        io_uring_sqe_set_data(sqe, &result);
        Async::submitRing();
    });
}

template <auto Tag>
Task<RecvResult> Delegates::Bidirectional<Tag>::recv(size_t size) {
    std::string data(size, 0);

    auto recvResult = co_await Async::run([this, &data](Async::CompletionResult& result) {
        io_uring_sqe* sqe = Async::getUringSQE();
        io_uring_prep_recv(sqe, *handle, data.data(), data.size(), MSG_NOSIGNAL);
        io_uring_sqe_set_data(sqe, &result);
        Async::submitRing();
    });

    if (recvResult.res == 0) co_return std::nullopt;

    data.resize(recvResult.res);
    co_return data;
}

template Task<> Delegates::Bidirectional<SocketTag::IP>::send(std::string);
template Task<RecvResult> Delegates::Bidirectional<SocketTag::IP>::recv(size_t);

template Task<> Delegates::Bidirectional<SocketTag::BT>::send(std::string);
template Task<RecvResult> Delegates::Bidirectional<SocketTag::BT>::recv(size_t);
#endif
