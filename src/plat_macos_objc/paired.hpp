// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_APPLE
#include <string>
#include <vector>

namespace ObjC {
    struct BluetoothDeviceInfo {
        std::string name;
        std::string addr;
    };

    std::vector<BluetoothDeviceInfo> getPaired();
}
#endif
