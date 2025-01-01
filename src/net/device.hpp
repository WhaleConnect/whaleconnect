// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>

#include "enums.hpp"

// Remote device metadata.
struct Device {
    ConnectionType type = ConnectionType::None; // Connection protocol
    std::string name; // Device name for display
    std::string address; // Address (IP address for TCP / UDP, MAC address for Bluetooth)
    std::uint16_t port = 0; // Port (or PSM for L2CAP, channel for RFCOMM)
};
