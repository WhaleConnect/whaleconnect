// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_APPLE
#include "btutils_internal.hpp"

#include <IOBluetooth/objc/IOBluetoothDevice.h>
#include <IOBluetooth/objc/IOBluetoothSDPServiceRecord.h>
#include <IOBluetooth/objc/IOBluetoothSDPUUID.h>

@interface InquiryHandler : NSObject <IOBluetoothDeviceAsyncCallbacks>
@property (readonly) IOReturn result;
- (void)remoteNameRequestComplete:(IOBluetoothDevice*)device status:(IOReturn)status;
- (void)connectionComplete:(IOBluetoothDevice*)device status:(IOReturn)status;
- (void)sdpQueryComplete:(IOBluetoothDevice*)device status:(IOReturn)status;
@end

@implementation InquiryHandler
@synthesize result;

- (void)remoteNameRequestComplete:(IOBluetoothDevice*)device status:(IOReturn)status {
}

- (void)connectionComplete:(IOBluetoothDevice*)device status:(IOReturn)status {
}

- (void)sdpQueryComplete:(IOBluetoothDevice*)device status:(IOReturn)status {
    result = status;
}

@end

void BTUtils::init() {}

void BTUtils::cleanup() {}

DeviceList BTUtils::getPaired() {
    NSArray* devices = [IOBluetoothDevice pairedDevices];

    DeviceList ret;
    for (IOBluetoothDevice* i in devices) {
        // Get name and address (dashes in address string are replaced with colons)
        NSString* name = [i name];
        NSString* addr = [[i addressString] stringByReplacingOccurrencesOfString:@"-" withString:@":"];
        ret.push_back({ ConnectionType::None, [name UTF8String], [addr UTF8String], 0 });
    }

    return ret;
}

BTUtils::SDPResultList BTUtils::sdpLookup(std::string_view addr, UUID128 uuid, bool flushCache) {
    auto device = [IOBluetoothDevice deviceWithAddressString:@(addr.data())];

    if (flushCache) {
        auto handler = [InquiryHandler new];
        NSArray* uuids = @[ [IOBluetoothSDPUUID uuidWithBytes:uuid.data() length:16] ];
        auto res = [device performSDPQuery:handler uuids:uuids];
    }

    SDPResultList ret;
    NSArray* services = [device services];
    for (IOBluetoothSDPServiceRecord* i in services) {

    }

    return ret;
}
#endif
