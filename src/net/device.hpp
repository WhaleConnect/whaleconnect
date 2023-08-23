// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "enums.hpp"

// A remote device's metadata.
struct Device {
    ConnectionType type = ConnectionType::None; // Connection protocol
    std::string name;                           // Device name for display
    std::string address;                        // Address (IP address for TCP / UDP, MAC address for Bluetooth)
    uint16_t port = 0;                          // Port (or PSM for L2CAP, channel for RFCOMM)
};

using DeviceList = std::vector<Device>;
