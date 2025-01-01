// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "device.hpp"
#include "utils/uuids.hpp"

namespace BTUtils {
    struct Instance {
        // Initializes the OS APIs to use Bluetooth.
        Instance();

        // Cleans up the OS APIs.
        ~Instance();
    };

    // Bluetooth profile descriptor.
    struct ProfileDesc {
        std::uint16_t uuid = 0; // 16-bit UUID
        std::uint8_t versionMajor = 0; // Major version number
        std::uint8_t versionMinor = 0; // Minor version number
    };

    // Service result returned from an SDP inquiry.
    struct SDPResult {
        std::vector<std::uint16_t> protoUUIDs; // 16-bit protocol UUIDs
        std::vector<UUIDs::UUID128> serviceUUIDs; // 128-bit service class UUIDs
        std::vector<ProfileDesc> profileDescs; // Profile descriptors
        std::uint16_t port = 0; // Port advertised (PSM for L2CAP, channel for RFCOMM)
        std::string name; // Service name
        std::string desc; // Service description (if any)
    };

    // Gets the Bluetooth devices that are paired to this computer.
    // The Device instances returned have no type set because the communication protocol to use with them is
    // indeterminate (the function doesn't know if L2CAP or RFCOMM should be used with each).
    std::vector<Device> getPaired();

    // Runs a Service Discovery Protocol (SDP) inquiry on a remote device.
    std::vector<SDPResult> sdpLookup(std::string_view addr, UUIDs::UUID128 uuid, bool flushCache);
}
