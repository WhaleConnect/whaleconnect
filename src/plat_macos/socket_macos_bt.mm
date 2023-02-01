// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <IOBluetooth/IOBluetooth.h>

#include "os/async.hpp"
#include "os/error.hpp"
#include "sockets/device.hpp"
#include "sockets/socket.hpp"
#include "sockets/traits.hpp"

static id toChannel(SocketTraits<SocketTag::BT>::HandleType handle) {
    id channel = nil;

    if (handle.type == ConnectionType::RFCOMM) channel = [IOBluetoothRFCOMMChannel withObjectID:handle.handle];
    else [IOBluetoothL2CAPChannel withObjectID:handle.handle];

    return channel;
}

template <>
void Socket<SocketTag::BT>::close() {
    [toChannel(_handle) closeChannel];

    _release();
}

template <>
Task<> WritableSocket<SocketTag::BT>::send(std::string data) const {
    co_await Async::run(
        [this, &data](Async::CompletionResult& result) {
            Async::submitIOBluetooth(_get().handle, result);
            [toChannel(_get()) writeAsync:data.data() length:data.size() refcon:nullptr];
        },
        System::ErrorType::IOReturn);
}

template <>
Task<std::string> WritableSocket<SocketTag::BT>::recv() const {
    co_await Async::run([this](Async::CompletionResult& result) { Async::submitIOBluetooth(_get().handle, result); },
                        System::ErrorType::IOReturn);

    co_return Async::getBluetoothReadResult(_get().handle);
}

template <>
void WritableSocket<SocketTag::BT>::cancelIO() const {
    Async::bluetoothComplete(_get().handle, kIOReturnAborted);
}
#endif
