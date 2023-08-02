// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include "os/async.hpp"
#include "os/errcheck.hpp"
#include "sockets/delegates/bidirectional.hpp"
#include "sockets/enums.hpp"
#include "utils/task.hpp"

template <auto Tag>
Task<> Delegates::Bidirectional<Tag>::send(std::string data) const {
    co_await Async::run([this, &data](Async::CompletionResult& result) {
        WSABUF buf{ static_cast<ULONG>(data.size()), data.data() };
        call(FN(WSASend, _handle, &buf, 1, nullptr, 0, &result, nullptr));
    });
}

template <auto Tag>
Task<std::optional<std::string>> Delegates::Bidirectional<Tag>::recv() const {
    std::string data(_recvLen, 0);

    auto recvResult = co_await Async::run([this, &data](Async::CompletionResult& result) {
        DWORD flags = 0;
        WSABUF buf{ static_cast<ULONG>(_recvLen), data.data() };
        call(FN(WSARecv, _handle, &buf, 1, nullptr, &flags, &result, nullptr));
    });

    // Check for disconnects
    if (recvResult.res == 0) co_return std::nullopt;

    // Resize string to received size
    data.resize(recvResult.res);
    co_return data;
}

template <auto Tag>
void Delegates::Bidirectional<Tag>::cancelIO() const {
    CancelIoEx(std::bit_cast<HANDLE>(_handle), nullptr);
}

template Task<> Delegates::Bidirectional<SocketTag::IP>::send(std::string) const;
template Task<std::optional<std::string>> Delegates::Bidirectional<SocketTag::IP>::recv() const;
template void Delegates::Bidirectional<SocketTag::IP>::cancelIO() const;

template Task<> Delegates::Bidirectional<SocketTag::BT>::send(std::string) const;
template Task<std::optional<std::string>> Delegates::Bidirectional<SocketTag::BT>::recv() const;
template void Delegates::Bidirectional<SocketTag::BT>::cancelIO() const;
#endif
