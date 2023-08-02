// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include "os/error.hpp"
#include "plat_macos/async_macos.hpp"
#include "sockets/delegates/bidirectional.hpp"
#include "sockets/enums.hpp"

template <>
Task<> Delegates::Bidirectional<SocketTag::BT>::send(std::string data) const {
    [_handle write:data];
    co_await Async::run(std::bind_front(Async::submitIOBluetooth, [_handle channelHash], Async::IOType::Send),
                        System::ErrorType::IOReturn);
}

template <>
Task<std::optional<std::string>> Delegates::Bidirectional<SocketTag::BT>::recv() const {
    co_await Async::run(std::bind_front(Async::submitIOBluetooth, [_handle channelHash], Async::IOType::Receive),
                        System::ErrorType::IOReturn);

    co_return Async::getBluetoothReadResult([_handle channelHash]);
}

template <>
void Delegates::Bidirectional<SocketTag::BT>::cancelIO() const {
    Async::bluetoothCancel([_handle channelHash]);
}
#endif
