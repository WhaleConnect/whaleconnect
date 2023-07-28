// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <functional>

#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "async_macos.hpp"

#include "os/errcheck.hpp"
#include "sockets/socket.hpp"

template <>
void Socket<SocketTag::IP>::close() const {
    shutdown(_handle, SHUT_RDWR);
    ::close(_handle);
}

template <>
Task<> WritableSocket<SocketTag::IP>::send(std::string data) const {
    co_await Async::run(std::bind_front(Async::submitKqueue, _get(), EVFILT_WRITE));

    call(FN(::send, _get(), data.data(), data.size(), 0));
}

template <>
Task<std::optional<std::string>> WritableSocket<SocketTag::IP>::recv() const {
    co_await Async::run(std::bind_front(Async::submitKqueue, _get(), EVFILT_READ));

    std::string data(_recvLen, 0);
    ssize_t recvLen = call(FN(::recv, _get(), data.data(), data.size(), 0));

    if (recvLen == 0) co_return std::nullopt;

    data.resize(recvLen);
    co_return data;
}

template <>
void WritableSocket<SocketTag::IP>::cancelIO() const {
    Async::cancelPending(_get());
}
#endif
