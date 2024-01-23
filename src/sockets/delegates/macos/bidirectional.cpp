// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <coroutine> // IWYU pragma: keep
#include <functional>
#include <optional>

#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

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

    check(::send(*handle, data.data(), data.size(), 0));
}

template <>
Task<RecvResult> Delegates::Bidirectional<SocketTag::IP>::recv(size_t size) {
    co_await Async::run(std::bind_front(Async::submitKqueue, *handle, Async::IOType::Receive));

    std::string data(size, 0);
    ssize_t recvLen = check(::recv(*handle, data.data(), data.size(), 0));

    if (recvLen == 0) co_return { true, true, "", std::nullopt };

    data.resize(recvLen);
    co_return { true, false, data, std::nullopt };
}

template <>
Task<> Delegates::Bidirectional<SocketTag::BT>::send(std::string data) {
    check((*handle)->write(data), checkZero, useReturnCode, System::ErrorType::IOReturn);
    co_await Async::run(std::bind_front(Async::submitIOBluetooth, (*handle)->getHash(), Async::IOType::Send),
        System::ErrorType::IOReturn);
}

template <>
Task<RecvResult> Delegates::Bidirectional<SocketTag::BT>::recv(size_t) {
    co_await Async::run(std::bind_front(Async::submitIOBluetooth, (*handle)->getHash(), Async::IOType::Receive),
        System::ErrorType::IOReturn);

    auto readResult = Async::getBluetoothReadResult((*handle)->getHash());
    co_return readResult ? RecvResult{ true, false, *readResult, std::nullopt }
                         : RecvResult{ true, true, "", std::nullopt };
}
