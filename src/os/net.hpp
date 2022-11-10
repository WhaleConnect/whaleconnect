// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "error.hpp"
#include "utils/task.hpp"

#if OS_WINDOWS
// Winsock header
#include <WinSock2.h>
#else
using SOCKET = int; // Socket type to match code on Windows
#endif

class Socket;

namespace Net {
    // All possible connection types.
    // L2CAP connections are not supported on Windows because of limitations with the Microsoft Bluetooth stack.
    enum class ConnectionType { TCP, UDP, L2CAPSeqPacket, L2CAPStream, L2CAPDgram, RFCOMM, None };

    // A remote device's metadata.
    struct DeviceData {
        ConnectionType type = ConnectionType::None; // Connection protocol
        std::string name;                           // Device name for display
        std::string address; // Address (IP address for TCP / UDP, MAC address for Bluetooth)
        uint16_t port = 0;   // Port (or PSM for L2CAP, channel for RFCOMM)
    };

    using DeviceDataList = std::vector<DeviceData>;

    // Checks if a ConnectionType is an Internet-based connection.
    inline bool connectionTypeIsIP(ConnectionType type) {
        using enum ConnectionType;
        return (type == TCP) || (type == UDP);
    }

    // Checks if a ConnectionType is a Bluetooth-based connection.
    inline bool connectionTypeIsBT(ConnectionType type) {
        using enum ConnectionType;
        return (type == L2CAPSeqPacket) || (type == L2CAPStream) || (type == L2CAPDgram) || (type == RFCOMM);
    }

    // Prepares the OS sockets for use by the application.
    void init();

    // Cleans up the OS sockets.
    void cleanup();

    // Creates a client socket and connects it to a server.
    Task<Socket> createClientSocket(const DeviceData& data);
}
