// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_MACOS && __OBJC__
#include <string>

#include <IOBluetooth/IOBluetooth.h>
#include <IOKit/IOReturn.h>

// Interface to handle operations and events RFCOMM and L2CAP channels.
// This interface also serves as the delegate to Bluetooth channels.
@interface BTHandle : NSObject <IOBluetoothL2CAPChannelDelegate, IOBluetoothRFCOMMChannelDelegate>
@property (nonatomic, strong) id channel;
@property (nonatomic) bool isL2CAP;

// Initializes with a channel pointer.
- (instancetype)initWithChannel:(id)bluetoothChannel;

// Registers this instance as a delegate to its contained channel.
- (void)registerAsDelegate;

// Closes the channel.
- (void)close;

// Writes a string to the channel.
- (void)write:(std::string)data;

// Gets the hash of the channel (used for identification in plain C++ code).
- (NSUInteger)channelHash;
@end
#endif
