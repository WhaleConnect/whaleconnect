// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include <WinSock2.h>

#include "os/async.hpp"
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
        EXPECT_NONERROR(WSASend, this->_handle, &buf, 1, nullptr, 0, &result, nullptr);
    });
}

template <auto Tag>
Task<std::string> WritableSocket<Tag>::recv() const {
    std::string data(_recvLen, 0);

    auto result = co_await Async::run([this, &data](Async::CompletionResult& result) {
        DWORD flags = 0;
        WSABUF buf{ static_cast<ULONG>(_recvLen), data.data() };
        EXPECT_NONERROR(WSARecv, this->_handle, &buf, 1, nullptr, &flags, &result, nullptr);
    });

    data.resize(result.res);
    co_return data;
}

template <auto Tag>
void WritableSocket<Tag>::cancelIO() const {
    CancelIoEx(std::bit_cast<HANDLE>(this->_handle), nullptr);
}

template void Socket<SocketTag::IP>::close();
template Task<> WritableSocket<SocketTag::IP>::send(std::string) const;
template Task<std::string> WritableSocket<SocketTag::IP>::recv() const;
template void WritableSocket<SocketTag::IP>::cancelIO() const;

template void Socket<SocketTag::BT>::close();
template Task<> WritableSocket<SocketTag::BT>::send(std::string) const;
template Task<std::string> WritableSocket<SocketTag::BT>::recv() const;
template void WritableSocket<SocketTag::IP>::cancelIO() const;
#endif
