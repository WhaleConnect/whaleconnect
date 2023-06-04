// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include <WinSock2.h>

#include "async_windows.hpp"

#include "os/errcheck.hpp"
#include "sockets/socket.hpp"

template <auto Tag>
void Socket<Tag>::close() {
    shutdown(_handle, SD_BOTH);
    closesocket(_handle);
    _release();
}

template <auto Tag>
Task<> WritableSocket<Tag>::send(std::string data) const {
    co_await Async::run([this, &data](Async::CompletionResult& result) {
        WSABUF buf{ static_cast<ULONG>(data.size()), data.data() };
        call(FN(WSASend, this->_get(), &buf, 1, nullptr, 0, &result, nullptr));
    });
}

template <auto Tag>
Task<std::optional<std::string>> WritableSocket<Tag>::recv() const {
    std::string data(_recvLen, 0);

    auto recvResult = co_await Async::run([this, &data](Async::CompletionResult& result) {
        DWORD flags = 0;
        WSABUF buf{ static_cast<ULONG>(_recvLen), data.data() };
        call(FN(WSARecv, this->_get(), &buf, 1, nullptr, &flags, &result, nullptr));
    });

    if (recvResult.res == 0) co_return std::nullopt;

    data.resize(recvResult.res);
    co_return data;
}

template <auto Tag>
void WritableSocket<Tag>::cancelIO() const {
    CancelIoEx(std::bit_cast<HANDLE>(this->_get()), nullptr);
}

template void Socket<SocketTag::IP>::close();
template Task<> WritableSocket<SocketTag::IP>::send(std::string) const;
template Task<std::optional<std::string>> WritableSocket<SocketTag::IP>::recv() const;
template void WritableSocket<SocketTag::IP>::cancelIO() const;

template void Socket<SocketTag::BT>::close();
template Task<> WritableSocket<SocketTag::BT>::send(std::string) const;
template Task<std::optional<std::string>> WritableSocket<SocketTag::BT>::recv() const;
template void WritableSocket<SocketTag::BT>::cancelIO() const;
#endif
