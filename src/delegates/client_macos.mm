// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <IOKit/IOReturn.h>

#include "client.hpp"

#include "net/bthandle.h"
#include "net/enums.hpp"
#include "os/async_macos.hpp"
#include "os/error.hpp"

template <>
Task<> Delegates::Client<SocketTag::BT>::connect(const Device& device) {
    auto target = [IOBluetoothDevice deviceWithAddressString:@(device.address.c_str())];
    IOReturn result = kIOReturnSuccess;
    id channel = nil;

    const char* fnName = "<opening channel>";

    using enum ConnectionType;
    switch (device.type) {
        case L2CAP:
            result = [target openL2CAPChannelAsync:&channel withPSM:device.port delegate:nil];
            break;
        case RFCOMM:
            result = [target openRFCOMMChannelAsync:&channel withChannelID:device.port delegate:nil];
            break;
        default:
            throw System::SystemError{ kIOReturnUnsupported, System::ErrorType::IOReturn, fnName };
    }

    if (result != kIOReturnSuccess) throw System::SystemError{ result, System::ErrorType::IOReturn, fnName };

    // Init channel
    _handle.reset([[BTHandle alloc] initWithChannel:channel]);
    [*_handle registerAsDelegate];

    NSUInteger hash = [*_handle channelHash];
    co_await Async::run(std::bind_front(Async::submitIOBluetooth, hash, Async::IOType::Send),
                        System::ErrorType::IOReturn);
}
#endif
