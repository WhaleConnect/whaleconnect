// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include "bidirectional.hpp"

#include "net/enums.hpp"
#include "os/async.hpp"
#include "os/errcheck.hpp"
#include "utils/task.hpp"

template <auto Tag>
Task<> Delegates::Bidirectional<Tag>::send(std::string data) {
    co_await Async::run([this, &data](Async::CompletionResult& result) {
        WSABUF buf{ static_cast<ULONG>(data.size()), data.data() };
        call(FN(WSASend, *handle, &buf, 1, nullptr, 0, &result, nullptr));
    });
}

template <auto Tag>
Task<RecvResult> Delegates::Bidirectional<Tag>::recv(size_t size) {
    std::string data(size, 0);

    auto recvResult = co_await Async::run([this, &data](Async::CompletionResult& result) {
        DWORD flags = 0;
        WSABUF buf{ static_cast<ULONG>(data.size()), data.data() };
        call(FN(WSARecv, *handle, &buf, 1, nullptr, &flags, &result, nullptr));
    });

    // Check for disconnects
    if (recvResult.res == 0) co_return std::nullopt;

    // Resize string to received size
    data.resize(recvResult.res);
    co_return data;
}

template Task<> Delegates::Bidirectional<SocketTag::IP>::send(std::string);
template Task<RecvResult> Delegates::Bidirectional<SocketTag::IP>::recv(size_t);

template Task<> Delegates::Bidirectional<SocketTag::BT>::send(std::string);
template Task<RecvResult> Delegates::Bidirectional<SocketTag::BT>::recv(size_t);
#endif
