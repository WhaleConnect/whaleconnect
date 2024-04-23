// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <WinSock2.h>

#include "os/async.hpp"
#include "sockets/delegates/bidirectional.hpp"
#include "utils/task.hpp"

template <auto Tag>
Task<> Delegates::Bidirectional<Tag>::send(std::string data) {
    co_await Async::run([this, &data](Async::CompletionResult& result) {
        WSABUF buf{ static_cast<ULONG>(data.size()), data.data() };
        Async::submit(Async::Send{ { *handle, &result }, &buf });
    });
}

template <auto Tag>
Task<RecvResult> Delegates::Bidirectional<Tag>::recv(std::size_t size) {
    std::string data(size, 0);

    auto recvResult = co_await Async::run([this, &data](Async::CompletionResult& result) {
        WSABUF buf{ static_cast<ULONG>(data.size()), data.data() };
        Async::submit(Async::Receive{ { *handle, &result }, &buf });
    });

    // Check for disconnects
    if (recvResult.res == 0) co_return { true, true, "", std::nullopt };

    // Resize string to received size
    data.resize(recvResult.res);
    co_return { true, false, data, std::nullopt };
}

template Task<> Delegates::Bidirectional<SocketTag::IP>::send(std::string);
template Task<RecvResult> Delegates::Bidirectional<SocketTag::IP>::recv(std::size_t);

template Task<> Delegates::Bidirectional<SocketTag::BT>::send(std::string);
template Task<RecvResult> Delegates::Bidirectional<SocketTag::BT>::recv(std::size_t);
