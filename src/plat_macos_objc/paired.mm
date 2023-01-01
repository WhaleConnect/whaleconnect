// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_APPLE
#include "paired.hpp"

#include <IOBluetooth/objc/IOBluetoothDevice.h>

std::vector<ObjC::BluetoothDeviceInfo> ObjC::getPaired() {
    NSArray* devices = [IOBluetoothDevice pairedDevices];

    std::vector<BluetoothDeviceInfo> ret;
    for (IOBluetoothDevice* i in devices) {
        // Get name and address (dashes in address string are replaced with colons)
        NSString* name = [i name];
        NSString* addr = [[i addressString] stringByReplacingOccurrencesOfString:@"-" withString:@":"];
        ret.push_back({ [name UTF8String], [addr UTF8String] });
    }

    return ret;
}
#endif
