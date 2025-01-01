// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sockets/delegates/bidirectional.hpp"

#include <string>

#include "net/enums.hpp"
#include "os/async.hpp"
#include "utils/task.hpp"

template <auto Tag>
Task<> Delegates::Bidirectional<Tag>::send(std::string data) {
    co_await Async::run([this, &data](Async::CompletionResult& result) {
        Async::submit(Async::Send{ { *handle, &result }, data });
    });
}

template <auto Tag>
Task<RecvResult> Delegates::Bidirectional<Tag>::recv(std::size_t size) {
    std::string data(size, 0);

    auto recvResult = co_await Async::run([this, &data](Async::CompletionResult& result) {
        Async::submit(Async::Receive{ { *handle, &result }, data });
    });

    if (recvResult.res == 0) co_return { true, true, "", std::nullopt };

    data.resize(recvResult.res);
    co_return { true, false, data, std::nullopt };
}

template Task<> Delegates::Bidirectional<SocketTag::IP>::send(std::string);
template Task<RecvResult> Delegates::Bidirectional<SocketTag::IP>::recv(std::size_t);

template Task<> Delegates::Bidirectional<SocketTag::BT>::send(std::string);
template Task<RecvResult> Delegates::Bidirectional<SocketTag::BT>::recv(std::size_t);
