// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_MACOS
#include <coroutine>
#include <functional>

#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "os/check.hpp"

module sockets.delegates.bidirectional;
import net.enums;
import os.async;
import os.async.platform;
import os.error;
import os.errcheck;
import utils.task;

template <>
Task<> Delegates::Bidirectional<SocketTag::IP>::send(std::string data) {
    co_await Async::run(std::bind_front(Async::submitKqueue, *handle, Async::IOType::Send));

    CHECK(::send(*handle, data.data(), data.size(), 0));
}

template <>
Task<RecvResult> Delegates::Bidirectional<SocketTag::IP>::recv(size_t size) {
    co_await Async::run(std::bind_front(Async::submitKqueue, *handle, Async::IOType::Receive));

    std::string data(size, 0);
    ssize_t recvLen = CHECK(::recv(*handle, data.data(), data.size(), 0));

    if (recvLen == 0) co_return std::nullopt;

    data.resize(recvLen);
    co_return data;
}

template <>
Task<> Delegates::Bidirectional<SocketTag::BT>::send(std::string data) {
    CHECK((*handle)->write(data), checkZero, useReturnCode, System::ErrorType::IOReturn);
    co_await Async::run(std::bind_front(Async::submitIOBluetooth, (*handle)->getHash(), Async::IOType::Send),
                        System::ErrorType::IOReturn);
}

template <>
Task<RecvResult> Delegates::Bidirectional<SocketTag::BT>::recv(size_t) {
    co_await Async::run(std::bind_front(Async::submitIOBluetooth, (*handle)->getHash(), Async::IOType::Receive),
                        System::ErrorType::IOReturn);

    co_return Async::getBluetoothReadResult((*handle)->getHash());
}
#endif
