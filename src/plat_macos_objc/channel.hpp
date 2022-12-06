// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_APPLE
#include <IOBluetooth/IOBluetoothUserLib.h>

namespace ObjC {
    void closeRFCOMMChannel(IOBluetoothObjectID channelID);

    void closeL2CAPChannel(IOBluetoothObjectID channelID);
}
#endif
