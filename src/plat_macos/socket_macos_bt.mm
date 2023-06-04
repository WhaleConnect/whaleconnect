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
void Socket<SocketTag::BT>::close() {
    [_handle close];

    _release();
}

template <>
Task<> WritableSocket<SocketTag::BT>::send(std::string data) const {
    co_await Async::run(
        [this, &data](Async::CompletionResult& result) {
            Async::submitIOBluetooth([_get() channelHash], Async::BluetoothIOType::Send, result);
            [_get() write:data];
        },
        System::ErrorType::IOReturn);
}

template <>
Task<std::optional<std::string>> WritableSocket<SocketTag::BT>::recv() const {
    co_await Async::run(
        std::bind_front(Async::submitIOBluetooth, [_get() channelHash], Async::BluetoothIOType::Receive),
        System::ErrorType::IOReturn);

    co_return Async::getBluetoothReadResult([_get() channelHash]);
}

template <>
void WritableSocket<SocketTag::BT>::cancelIO() const {
    auto channelHash = [_get() channelHash];
    Async::bluetoothComplete(channelHash, Async::BluetoothIOType::Send, kIOReturnAborted);
    Async::bluetoothComplete(channelHash, Async::BluetoothIOType::Receive, kIOReturnAborted);
}
#endif
