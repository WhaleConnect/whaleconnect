// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief Bluetooth utility functions
*/

#pragma once

#include <string> // std::string
#include <vector> // std::vector

#include "sockets.hpp"
#include "sys/error.hpp"

#if OS_WINDOWS
#include <guiddef.h>
#else
/**
 * @brief A GUID structure. Provided on Linux for compatibility with the Windows API.
*/
struct GUID {
    unsigned long Data1; /**< The first 8 hex digits */
    unsigned short Data2; /**< The first group of 4 hex digits */
    unsigned short Data3; /**< The second group of 4 hex digits */
    unsigned char Data4[8]; /**< The last hex digits (the third group of 4, and the last 12) */
};
#endif

/**
 * @brief An alias for a pointer to a @p UUID.
*/
using LPUUID = GUID*;

namespace BTUtils {
    /**
     * @brief A Bluetooth profile descriptor.
    */
    struct ProfileDesc {
        uint16_t uuid = 0; /**< The profile's 16-bit UUID */
        uint8_t versionMajor = 0; /**< The profile's major version number */
        uint8_t versionMinor = 0; /**< The profile's minor version number */
    };

    /**
     * @brief A single service result returned from an SDP inquiry.
    */
    struct SDPResult {
        std::vector<uint16_t> protoUUIDs; /**< The list of 16-bit protocol UUIDs */
        std::vector<GUID> serviceUUIDs; /**< The list of 128-bit service class UUIDs */
        std::vector<ProfileDesc> profileDescs; /**< The list of profile descriptors */
        uint16_t port = 0; /**< The port advertised (PSM for L2CAP, channel for RFCOMM) */
        std::string name; /**< The name of the service */
        std::string desc; /**< The description of the service (if any) */
    };

    /**
     * @brief A vector of @p SDPResult structs.
    */
    using SDPResultList = std::vector<SDPResult>;

    /**
     * @brief Initializes the OS APIs to use Bluetooth.
    */
    void init();

    /**
     * @brief Cleans up the OS APIs.
    */
    void cleanup();

    /**
     * @brief Constructs a 128-bit Bluetooth UUID given the short UUID.
     * @param uuidShort A 16- or 32-bit UUID value
     * @return The 128-bit UUID constructed from merging the given UUID with the Bluetooth base UUID
    */
    constexpr GUID createUUIDFromBase(uint32_t uuidShort) {
        // To turn a 16-bit UUID into a 128-bit UUID:
        //   | The 16-bit Attribute UUID replaces the x's in the following:
        //   | 0000xxxx - 0000 - 1000 - 8000 - 00805F9B34FB
        // https://stackoverflow.com/a/36212021
        // (The same applies with a 32-bit UUID)
        return { uuidShort, 0x0000, 0x1000, { 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } };
    }

    /**
     * @brief Gets the Bluetooth devices that are paired to this computer.
     * @return A vector containing each device
     *
     * The @p DeviceData instances returned have no type set because the communication protocol to use with them is
     * indeterminate (the function doesn't know if L2CAP or RFCOMM should be used with each).
    */
    Sockets::DeviceDataList getPaired();

    /**
     * @brief Runs a Service Discovery Protocol (SDP) inquiry on a remote device.
     * @param addr The MAC address of the device
     * @param uuid The UUID of the service to inquiry
     * @param flushCache If previous caches are ignored during the inquiry (Windows only)
     * @return A vector of each service found
    */
    SDPResultList sdpLookup(std::string_view addr, GUID uuid, bool flushCache);
}
