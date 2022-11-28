// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_APPLE
#include <functional>

#include <IOBluetooth/IOBluetooth.h>
#include <sys/event.h>
#include <sys/socket.h>

#include "os/async.hpp"
#include "os/errcheck.hpp"
#include "socket.hpp"

template <>
void Socket<SocketTag::IP>::close() {
    shutdown(_handle, SHUT_RDWR);
    ::close(_handle);
    _release();
}

template <>
Task<> WritableSocket<SocketTag::IP>::send(std::string data) const {
    co_await Async::run(std::bind_front(Async::submitKqueue, _handle, EVFILT_WRITE));

    EXPECT_NONERROR(::send, _handle, data.data(), data.size(), 0);
}

template <>
Task<std::string> WritableSocket<SocketTag::IP>::recv() const {
    std::string data(_recvLen, 0);

    auto result = co_await Async::run(std::bind_front(Async::submitKqueue, _handle, EVFILT_READ));

    EXPECT_NONERROR(::recv, _handle, data.data(), data.size(), 0);

    data.resize(result.res);
    co_return data;
}

template <>
void Socket<SocketTag::BT>::close() {
    if (_handle.type == ConnectionType::RFCOMM) {
        auto channel = [IOBluetoothRFCOMMChannel withObjectID:*_handle];
        [channel closeChannel];
        [[channel getDevice] closeConnection];
    } else {
        auto channel = [IOBluetoothL2CAPChannel withObjectID:*_handle];
        [channel closeChannel];
        [[channel device] closeConnection];
    }

    _release();
}

template <>
Task<> WritableSocket<SocketTag::BT>::send(std::string) const {
    co_return;
}

template <>
Task<std::string> WritableSocket<SocketTag::BT>::recv() const {
    co_return "";
}
#endif
