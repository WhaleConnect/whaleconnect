// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_APPLE
#include "os/btutils_internal.hpp"

#include <IOBluetooth/objc/IOBluetoothDevice.h>
#include <IOBluetooth/objc/IOBluetoothSDPServiceRecord.h>
#include <IOBluetooth/objc/IOBluetoothSDPUUID.h>

#include "os/btutils.hpp"
#include "sockets/device.hpp"
#include "sockets/traits.hpp"

@interface InquiryHandler : NSObject <IOBluetoothDeviceAsyncCallbacks>
@property (readonly) IOReturn result;
@property (readonly, strong) NSCondition* cond;
@end

@implementation InquiryHandler
@synthesize result;
@synthesize cond;

- (void)remoteNameRequestComplete:(IOBluetoothDevice*)device status:(IOReturn)status {
}

- (void)connectionComplete:(IOBluetoothDevice*)device status:(IOReturn)status {
}

- (void)sdpQueryComplete:(IOBluetoothDevice*)device status:(IOReturn)status {
    result = status;
    [cond signal];
    [cond unlock];
}

@end

BTUtils::Instance::Instance() {}

BTUtils::Instance::~Instance() {}

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
        IOReturn res = [device performSDPQuery:handler uuids:uuids];
        if (res != kIOReturnSuccess) return {};

        [[handler cond] lock];
        [[handler cond] wait];
        if ([handler result] != kIOReturnSuccess) return {};
    }

    NSArray* services = [device services];
    for (IOBluetoothSDPServiceRecord* i in services) {}
}
#endif
