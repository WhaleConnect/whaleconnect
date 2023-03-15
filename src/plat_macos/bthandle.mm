// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include "bthandle.h"

#include "async_macos.hpp"
#include "os/error.hpp"

static void newData(id channel, const char* data, size_t dataLen) {
    Async::bluetoothReadComplete([channel hash], data, dataLen);
}

static void outgoingComplete(id channel, IOReturn status) {
    Async::bluetoothComplete([channel hash], Async::BluetoothIOType::Send, status);
}

static void check(IOReturn code, const char* fnName) {
    if (code != kIOReturnSuccess) throw System::SystemError{ code, System::ErrorType::IOReturn, fnName };
}

@implementation BTHandle

- (instancetype)initWithChannel:(id)bluetoothChannel {
    self = [super init];
    if (self) _channel = bluetoothChannel;

    return self;
}

- (void)registerAsDelegate {
    IOReturn res;

    if ([_channel isKindOfClass:[IOBluetoothRFCOMMChannel class]])
        res = [static_cast<IOBluetoothRFCOMMChannel*>(_channel) setDelegate:self];
    else res = [static_cast<IOBluetoothL2CAPChannel*>(_channel) setDelegate:self];

    check(res, "setDelegate");
}

- (void)close {
    [_channel closeChannel];
}

- (void)write:(std::string)data {
    IOReturn res = [_channel writeAsync:data.data() length:data.size() refcon:nil];
    check(res, "writeAsync");
}

- (NSUInteger)channelHash {
    return [_channel hash];
}

- (void)l2capChannelClosed:(IOBluetoothL2CAPChannel*)l2capChannel {
    newData(l2capChannel, "", 0);
}

- (void)l2capChannelData:(IOBluetoothL2CAPChannel*)l2capChannel data:(void*)dataPointer length:(size_t)dataLength {
    newData(l2capChannel, static_cast<const char*>(dataPointer), dataLength);
}

- (void)l2capChannelOpenComplete:(IOBluetoothL2CAPChannel*)l2capChannel status:(IOReturn)error {
    outgoingComplete(l2capChannel, error);
}

- (void)l2capChannelWriteComplete:(IOBluetoothL2CAPChannel*)l2capChannel refcon:(void*)refcon status:(IOReturn)error {
    outgoingComplete(l2capChannel, error);
}

- (void)rfcommChannelClosed:(IOBluetoothRFCOMMChannel*)rfcommChannel {
    newData(rfcommChannel, "", 0);
}

- (void)rfcommChannelData:(IOBluetoothRFCOMMChannel*)rfcommChannel data:(void*)dataPointer length:(size_t)dataLength {
    newData(rfcommChannel, static_cast<const char*>(dataPointer), dataLength);
}

- (void)rfcommChannelOpenComplete:(IOBluetoothRFCOMMChannel*)rfcommChannel status:(IOReturn)error {
    outgoingComplete(rfcommChannel, error);
}

- (void)rfcommChannelWriteComplete:(IOBluetoothRFCOMMChannel*)rfcommChannel
                            refcon:(void*)refcon
                            status:(IOReturn)error {
    outgoingComplete(rfcommChannel, error);
}

@end
#endif
