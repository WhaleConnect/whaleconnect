// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sockets/delegates/bidirectional.hpp"

#include <functional>
#include <optional>
#include <string>

#include <BluetoothMacOS-Swift.h>
#include <sys/socket.h>

#include "net/enums.hpp"
#include "os/async.hpp"
#include "os/bluetooth.hpp"
#include "os/errcheck.hpp"
#include "os/error.hpp"
#include "utils/task.hpp"

template <>
Task<> Delegates::Bidirectional<SocketTag::IP>::send(std::string data) {
    co_await Async::run(
        [this](Async::CompletionResult& result) { Async::submit(Async::Send{ { *handle, &result } }); });

    check(::send(*handle, data.data(), data.size(), 0));
}

template <>
Task<RecvResult> Delegates::Bidirectional<SocketTag::IP>::recv(std::size_t size) {
    co_await Async::run(
        [this](Async::CompletionResult& result) { Async::submit(Async::Receive{ { *handle, &result } }); });

    std::string data(size, 0);
    auto recvLen = check(::recv(*handle, data.data(), data.size(), 0));

    if (recvLen == 0) co_return { true, true, "", std::nullopt };

    data.resize(recvLen);
    co_return { true, false, data, std::nullopt };
}

template <>
Task<> Delegates::Bidirectional<SocketTag::BT>::send(std::string data) {
    check((*handle)->write(data), checkZero, useReturnCode, System::ErrorType::IOReturn);
    co_await Async::run(std::bind_front(AsyncBT::submit, (*handle)->getHash(), IOType::Send),
        System::ErrorType::IOReturn);
}

template <>
Task<RecvResult> Delegates::Bidirectional<SocketTag::BT>::recv(std::size_t) {
    co_await Async::run(std::bind_front(AsyncBT::submit, (*handle)->getHash(), IOType::Receive),
        System::ErrorType::IOReturn);

    auto readResult = AsyncBT::getReadResult((*handle)->getHash());
    co_return readResult ? RecvResult{ true, false, *readResult, std::nullopt }
                         : RecvResult{ true, true, "", std::nullopt };
}
