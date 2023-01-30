// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include "os/btutils_internal.hpp"

#include <Foundation/Foundation.h>
#include <IOBluetooth/IOBluetooth.h>
#include <IOKit/IOReturn.h>

#include "os/btutils.hpp"
#include "sockets/device.hpp"
#include "sockets/traits.hpp"

@interface InquiryHandler : NSObject <IOBluetoothDeviceAsyncCallbacks>
@property bool finished;
@property (readonly) IOReturn result;
@property (readonly, strong) NSCondition* cond;

- (IOReturn)wait;
@end

@implementation InquiryHandler

- (void)remoteNameRequestComplete:(IOBluetoothDevice*)device status:(IOReturn)status {
}

- (void)connectionComplete:(IOBluetoothDevice*)device status:(IOReturn)status {
}

- (void)sdpQueryComplete:(IOBluetoothDevice*)device status:(IOReturn)status {
    [_cond lock];
    _finished = true;
    _result = status;
    [_cond signal];
    [_cond unlock];
}

- (IOReturn)wait {
    [_cond lock];
    _finished = false;

    while (!_finished) [_cond wait];

    IOReturn res = _result;
    [_cond unlock];

    return res;
}

@end

BTUtils::Instance::Instance() {}

BTUtils::Instance::~Instance() {}

void BTUtils::processAsyncEvents() {
    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
}

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

        if ([handler wait] != kIOReturnSuccess) return {};
    }

    NSArray* services = [device services];
    for (IOBluetoothSDPServiceRecord* i in services) {}
}
#endif
