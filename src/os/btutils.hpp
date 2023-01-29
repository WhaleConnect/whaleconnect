// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <string>
#include <vector>

// For htonl()
#if OS_WINDOWS
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#endif

#include "sockets/device.hpp"

namespace BTUtils {
    // A 128-bit UUID represented in a platform-independent way.
    using UUID128 = std::array<uint8_t, 16>;

    struct Instance {
        // Initializes the OS APIs to use Bluetooth.
        Instance();

        // Cleans up the OS APIs.
        ~Instance();
    };

    // A Bluetooth profile descriptor.
    struct ProfileDesc {
        uint16_t uuid = 0;        // 16-bit UUID
        uint8_t versionMajor = 0; // Major version number
        uint8_t versionMinor = 0; // Minor version number
    };

    // A single service result returned from an SDP inquiry.
    struct SDPResult {
        std::vector<uint16_t> protoUUIDs;      // 16-bit protocol UUIDs
        std::vector<UUID128> serviceUUIDs;     // 128-bit service class UUIDs
        std::vector<ProfileDesc> profileDescs; // Profile descriptors
        uint16_t port = 0;                     // Port advertised (PSM for L2CAP, channel for RFCOMM)
        std::string name;                      // Service name
        std::string desc;                      // Service description (if any)
    };

    using SDPResultList = std::vector<SDPResult>;

    // Constructs a 128-bit Bluetooth UUID given the short (16- or 32-bit) UUID.
    inline UUID128 createUUIDFromBase(uint32_t uuidShort) {
        // To turn a 16-bit UUID into a 128-bit UUID:
        //   | The 16-bit Attribute UUID replaces the x's in the following:
        //   | 0000xxxx - 0000 - 1000 - 8000 - 00805F9B34FB
        // https://stackoverflow.com/a/36212021
        // (The same applies with a 32-bit UUID)
        uint32_t uuid32 = htonl(uuidShort);
        uint8_t base[] = { 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB };

        UUID128 ret;
        std::memcpy(ret.data(), &uuid32, 4);
        std::memcpy(ret.data() + 4, base, 12);
        return ret;
    }

    // Gets the Bluetooth devices that are paired to this computer.
    //
    // The Device instances returned have no type set because the communication protocol to use with them is
    // indeterminate (the function doesn't know if L2CAP or RFCOMM should be used with each).
    DeviceList getPaired();

    // Runs a Service Discovery Protocol (SDP) inquiry on a remote device.
    SDPResultList sdpLookup(std::string_view addr, UUID128 uuid, bool flushCache);
}
