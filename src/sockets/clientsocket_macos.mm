// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <IOBluetooth/IOBluetooth.h>
#include <IOKit/IOReturn.h>

#include "clientsocket.hpp"

#include "net/bthandle.h"
#include "os/async_macos.hpp"
#include "os/error.hpp"

template <>
void ClientSocketBT::_init() {
    const auto& device = _traits.device;

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

    // Init channel without delegate
    _handle = [[BTHandle alloc] initWithChannel:channel];
}
#endif
