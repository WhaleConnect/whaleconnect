// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include <WinSock2.h>
#include <MSWSock.h>

#include "async.hpp"
#include "errcheck.hpp"
#include "socket.hpp"

template <>
void SocketBase<SOCKET>::close() {
    shutdown(_handle, SD_BOTH);
    closesocket(_handle);
    _release();
}

template <>
Task<> SocketBase<SOCKET>::send(std::string data) const {
    co_await Async::run([this, &data](Async::CompletionResult& result) {
        WSABUF buf{ static_cast<ULONG>(data.size()), data.data() };
        EXPECT_NONERROR(WSASend, _handle, &buf, 1, nullptr, 0, &result, nullptr);
    });
}

template <>
Task<std::string> SocketBase<SOCKET>::recv() const {
    std::string data(_recvLen, 0);

    auto result = co_await Async::run([this, &data](Async::CompletionResult& result) {
        DWORD flags = 0;
        WSABUF buf{ static_cast<ULONG>(_recvLen), data.data() };
        EXPECT_NONERROR(WSARecv, _handle, &buf, 1, nullptr, &flags, &result, nullptr);
    });

    data.resize(result.res);
    co_return data;
}
#endif
