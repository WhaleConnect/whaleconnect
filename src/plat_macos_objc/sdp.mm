// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_APPLE
#include "sdp.hpp"

#include <IOBluetooth/objc/IOBluetoothDevice.h>
#include <IOBluetooth/objc/IOBluetoothSDPServiceRecord.h>
#include <IOBluetooth/objc/IOBluetoothSDPUUID.h>

@interface InquiryHandler : NSObject <IOBluetoothDeviceAsyncCallbacks>
@property (readonly) IOReturn result;
@property (readonly, strong) NSCondition* cond;
- (void)remoteNameRequestComplete:(IOBluetoothDevice*)device status:(IOReturn)status;
- (void)connectionComplete:(IOBluetoothDevice*)device status:(IOReturn)status;
- (void)sdpQueryComplete:(IOBluetoothDevice*)device status:(IOReturn)status;
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

IOReturn ObjC::sdpLookup(const char* addr, const uint8_t* uuid, bool flushCache) {
    auto device = [IOBluetoothDevice deviceWithAddressString:@(addr)];

    if (flushCache) {
        auto handler = [InquiryHandler new];
        NSArray* uuids = @[ [IOBluetoothSDPUUID uuidWithBytes:uuid length:16] ];
        IOReturn res = [device performSDPQuery:handler uuids:uuids];
        if (res != kIOReturnSuccess) return res;

        [[handler cond] lock];
        [[handler cond] wait];
        if ([handler result] != kIOReturnSuccess) return [handler result];
    }

    NSArray* services = [device services];
    for (IOBluetoothSDPServiceRecord* i in services) {

    }

    return kIOReturnSuccess;
}
#endif
