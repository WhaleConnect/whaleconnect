// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_MACOS && __OBJC__
#include <string>

#include <IOBluetooth/IOBluetooth.h>
#include <IOKit/IOReturn.h>

@interface BTHandle : NSObject <IOBluetoothL2CAPChannelDelegate, IOBluetoothRFCOMMChannelDelegate>
@property (nonatomic, strong) id channel;
@property (nonatomic) bool isL2CAP;

- (instancetype)initWithChannel:(id)bluetoothChannel;

- (void)registerAsDelegate;

- (void)close;

- (void)write:(std::string)data;

- (NSUInteger)channelHash;
@end
#endif
