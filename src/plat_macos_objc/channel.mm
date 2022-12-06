// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_APPLE
#include <IOBluetooth/IOBluetooth.h>

#include "channel.hpp"

void ObjC::closeRFCOMMChannel(IOBluetoothObjectID channelID) {
    auto channel = [IOBluetoothRFCOMMChannel withObjectID:channelID];
    [channel closeChannel];
    [[channel getDevice] closeConnection];
}

void ObjC::closeL2CAPChannel(IOBluetoothObjectID channelID) {
    auto channel = [IOBluetoothL2CAPChannel withObjectID:channelID];
    [channel closeChannel];
    [[channel device] closeConnection];
}
#endif
