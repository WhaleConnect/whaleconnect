// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Bluetooth utility functions for Windows and Linux

#pragma once

#include <string> // std::string
#include <vector> // std::vector

#include "sockets.hpp"
#include "sys/error.hpp"

#ifndef _WIN32
/// <summary>
/// A UUID structure. Provided on Linux for compatibility with Winsock.
/// </summary>
struct UUID {
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
};
#endif

// Alias for pointer to type `UUID`
using LPUUID = UUID*;

/// <summary>
/// Namespace containing various Bluetooth utilities.
/// </summary>
namespace BTUtils {
    /// <summary>
    /// Initialize the OS APIs to use Bluetooth.
    /// </summary>
    void init();

    /// <summary>
    /// Clean up the OS APIs.
    /// </summary>
    void cleanup();

    /// <summary>
    /// A Bluetooth profile descriptor, consisting of a standard 16-bit UUID and the major/minor version numbers.
    /// </summary>
    struct ProfileDesc {
        uint16_t uuid = 0;
        uint8_t versionMajor = 0;
        uint8_t versionMinor = 0;
    };

    /// <summary>
    /// A single service result returned from an SDP inquiry.
    /// </summary>
    struct SDPResult {
        std::vector<uint16_t> protoUUIDs; // List of 16-bit protocol UUIDs
        std::vector<UUID> serviceUUIDs; // List of 128-bit service class UUIDs
        std::vector<ProfileDesc> profileDescs; // List of profile descriptors
        uint16_t port = 0; // The port advertised (channel for RFCOMM, PSM for L2CAP)
        std::string name; // The name of the service
        std::string desc; // The description of the service (if any)
    };

    // A vector of SDPResult structs
    using SDPResultList = std::vector<SDPResult>;

    /// <summary>
    /// Check if Bluetooth is initialized.
    /// </summary>
    /// <returns>
    /// On Linux: If the app has a DBus connection to bluetoothd, on Windows: true (no init on Windows)
    /// </returns>
    bool initialized();

    /// <summary>
    /// Construct a 128-bit UUID for a service given the 16- or 32-bit UUID using the Bluetooth base UUID.
    /// </summary>
    /// <param name="uuidShort">The UUID (short form) to create a 128-bit UUID from</param>
    /// <returns>The 128-bit UUID constructed from merging the given short UUID with the Bluetooth base UUID</returns>
    constexpr UUID createUUIDFromBase(uint32_t uuidShort) {
        // To turn a 16-bit UUID into a 128-bit UUID:
        //   | The 16-bit Attribute UUID replaces the x’s in the following:
        //   | 0000xxxx - 0000 - 1000 - 8000 - 00805F9B34FB
        // https://stackoverflow.com/a/36212021
        // (The same applies with a 32-bit UUID)
        return { uuidShort, 0x0000, 0x1000, { 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } };
    }

    /// <summary>
    /// Get the Bluetooth devices that are paired to this computer.
    /// </summary>
    /// <returns>A vector containing each device</returns>
    /// <remarks>
    /// The DeviceData instances returned have no type set because the communication protocol to use with them
    /// is indeterminate (the function doesn't know if L2CAP or RFCOMM should be used with each).
    /// </remarks>
    Sockets::DeviceDataList getPaired();

    /// <summary>
    /// Run a Service Discovery Protocol (SDP) inquiry on a remote device.
    /// </summary>
    /// <param name="addr">The MAC address of the device</param>
    /// <param name="uuid">The UUID of the service to inquiry</param>
    /// <param name="flushCache">If previous caches are ignored during the inquiry (Windows only)</param>
    /// <returns>A vector of each service found</returns>
    SDPResultList sdpLookup(std::string_view addr, const UUID& uuid, bool flushCache);
}
