// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <IOBluetooth/IOBluetooth.h>

#include "async_macos.hpp"

#include "os/error.hpp"
#include "sockets/device.hpp"
#include "sockets/socket.hpp"
#include "sockets/traits.hpp"

template <>
void Socket<SocketTag::BT>::close() const {
    [_handle close];
}

template <>
Task<> WritableSocket<SocketTag::BT>::send(std::string data) const {
    [_get() write:data];
    co_await Async::run(std::bind_front(Async::submitIOBluetooth, [_get() channelHash], Async::IOType::Send),
                        System::ErrorType::IOReturn);
}

template <>
Task<std::optional<std::string>> WritableSocket<SocketTag::BT>::recv() const {
    co_await Async::run(std::bind_front(Async::submitIOBluetooth, [_get() channelHash], Async::IOType::Receive),
                        System::ErrorType::IOReturn);

    co_return Async::getBluetoothReadResult([_get() channelHash]);
}

template <>
void WritableSocket<SocketTag::BT>::cancelIO() const {
    Async::bluetoothCancel([_get() channelHash]);
}
#endif
