// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_APPLE
#include <IOBluetooth/IOBluetooth.h>

#include "channel.hpp"

void ObjC::closeRFCOMMChannel(IOBluetoothObjectID channelID) {
    auto channel = [IOBluetoothRFCOMMChannel withObjectID:channelID];
    [channel closeChannel];
}

void ObjC::closeL2CAPChannel(IOBluetoothObjectID channelID) {
    auto channel = [IOBluetoothL2CAPChannel withObjectID:channelID];
    [channel closeChannel];
}
#endif
