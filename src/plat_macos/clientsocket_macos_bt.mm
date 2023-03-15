// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <IOBluetooth/IOBluetooth.h>
#include <IOKit/IOReturn.h>

#include "async_macos.hpp"
#include "bthandle.h"

#include "os/error.hpp"
#include "sockets/clientsocket.hpp"
#include "sockets/traits.hpp"

template <>
std::unique_ptr<ClientSocket<SocketTag::BT>> createClientSocket(const Device& device) {
    auto target = [IOBluetoothDevice deviceWithAddressString:@(device.address.c_str())];
    IOReturn result = kIOReturnSuccess;
    id channel = nil;

    const char* fnName = "<opening channel>";

    using enum ConnectionType;
    switch (device.type) {
        case L2CAPStream: result = [target openL2CAPChannelAsync:&channel withPSM:device.port delegate:nil]; break;
        case RFCOMM: result = [target openRFCOMMChannelAsync:&channel withChannelID:device.port delegate:nil]; break;
        default: throw System::SystemError{ kIOReturnUnsupported, System::ErrorType::IOReturn, fnName };
    }

    if (result != kIOReturnSuccess) throw System::SystemError{ result, System::ErrorType::IOReturn, fnName };

    auto handle = [[BTHandle alloc] initWithChannel:channel];
    return std::make_unique<ClientSocket<SocketTag::BT>>(handle, device);
}

template <>
Task<> ClientSocket<SocketTag::BT>::connect() const {
    [_get() registerAsDelegate];

    NSUInteger hash = [_get() channelHash];
    co_await Async::run(std::bind_front(Async::submitIOBluetooth, hash, Async::BluetoothIOType::Send));
}
#endif
