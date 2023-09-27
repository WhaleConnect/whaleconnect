// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include "cpp_objc_bridge.hpp"

uint64_t CppObjCBridge::getChannelHash(IOBluetoothObject* channel) {
    return [channel hash];
}

uint64_t CppObjCBridge::getBTHandleHash(BTHandle *handle) {
    return [handle channelHash];
}

BTHandle* CppObjCBridge::Bluetooth::connect(std::string_view address, uint16_t port, bool isL2CAP) {
    auto target = [IOBluetoothDevice deviceWithAddressString:@(address.data())];
    IOReturn result = kIOReturnSuccess;
    id channel = nil;

    if (isL2CAP) result = [target openL2CAPChannelAsync:&channel withPSM:port delegate:nil];
    else result = [target openRFCOMMChannelAsync:&channel withChannelID:port delegate:nil];

    if (result != kIOReturnSuccess) throwSystemError(result, "<opening channel>");

    BTHandle* handle = [[BTHandle alloc] initWithChannel:channel];
    [handle registerAsDelegate];

    return handle;
}

void CppObjCBridge::Bluetooth::write(BTHandle* handle, std::string_view data) {
    [handle write:data.data()];
}

void CppObjCBridge::Bluetooth::close(BTHandle* handle) {
    [handle close];
}
#endif
