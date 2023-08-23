// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <functional>

#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "bidirectional.hpp"

#include "os/async_macos.hpp"
#include "os/errcheck.hpp"
#include "sockets/enums.hpp"

template <>
Task<> Delegates::Bidirectional<SocketTag::IP>::send(std::string data) {
    co_await Async::run(std::bind_front(Async::submitKqueue, _handle, Async::IOType::Send));

    call(FN(::send, _handle, data.data(), data.size(), 0));
}

template <>
Task<RecvResult> Delegates::Bidirectional<SocketTag::IP>::recv() {
    co_await Async::run(std::bind_front(Async::submitKqueue, _handle, Async::IOType::Receive));

    std::string data(_recvLen, 0);
    ssize_t recvLen = call(FN(::recv, _handle, data.data(), data.size(), 0));

    if (recvLen == 0) co_return std::nullopt;

    data.resize(recvLen);
    co_return data;
}
#endif
