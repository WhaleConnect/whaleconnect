// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include "bthandle.h"

#include "cpp_objc_bridge.hpp"

@implementation BTHandle

- (instancetype)initWithChannel:(id)bluetoothChannel {
    self = [super init];
    if (self) {
        _channel = bluetoothChannel;
        _isL2CAP = [_channel isKindOfClass:[IOBluetoothL2CAPChannel class]];

        // Clear results of previous receive operations (includes notifications from channel closures)
        CppObjCBridge::Async::clearDataQueue(_channel);
    }

    return self;
}

- (void)registerAsDelegate {
    IOReturn res;

    if (_isL2CAP) res = [static_cast<IOBluetoothL2CAPChannel*>(_channel) setDelegate:self];
    else res = [static_cast<IOBluetoothRFCOMMChannel*>(_channel) setDelegate:self];

    if (res != kIOReturnSuccess) CppObjCBridge::throwSystemError(res, "setDelegate");
}

- (void)close {
    [_channel closeChannel];
}

- (void)write:(std::string)data {
    IOReturn res = [_channel writeAsync:data.data() length:data.size() refcon:nil];
    if (res != kIOReturnSuccess) CppObjCBridge::throwSystemError(res, "writeAsync");
}

- (NSUInteger)channelHash {
    return [_channel hash];
}

// Implementation methods for this interface to conform to IOBluetooth delegate protocols.

- (void)l2capChannelClosed:(IOBluetoothL2CAPChannel*)l2capChannel {
    CppObjCBridge::Async::closed(l2capChannel);
}

- (void)l2capChannelData:(IOBluetoothL2CAPChannel*)l2capChannel data:(void*)dataPointer length:(size_t)dataLength {
    CppObjCBridge::Async::newData(l2capChannel, static_cast<const char*>(dataPointer), dataLength);
}

- (void)l2capChannelOpenComplete:(IOBluetoothL2CAPChannel*)l2capChannel status:(IOReturn)error {
    CppObjCBridge::Async::outgoingComplete(l2capChannel, error);
}

- (void)l2capChannelWriteComplete:(IOBluetoothL2CAPChannel*)l2capChannel refcon:(void*)refcon status:(IOReturn)error {
    CppObjCBridge::Async::outgoingComplete(l2capChannel, error);
}

- (void)rfcommChannelClosed:(IOBluetoothRFCOMMChannel*)rfcommChannel {
    CppObjCBridge::Async::closed(rfcommChannel);
}

- (void)rfcommChannelData:(IOBluetoothRFCOMMChannel*)rfcommChannel data:(void*)dataPointer length:(size_t)dataLength {
    CppObjCBridge::Async::newData(rfcommChannel, static_cast<const char*>(dataPointer), dataLength);
}

- (void)rfcommChannelOpenComplete:(IOBluetoothRFCOMMChannel*)rfcommChannel status:(IOReturn)error {
    CppObjCBridge::Async::outgoingComplete(rfcommChannel, error);
}

- (void)rfcommChannelWriteComplete:(IOBluetoothRFCOMMChannel*)rfcommChannel
                            refcon:(void*)refcon
                            status:(IOReturn)error {
    CppObjCBridge::Async::outgoingComplete(rfcommChannel, error);
}

@end
#endif
