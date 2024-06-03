// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Enumeration to determine socket types at compile time.
enum class SocketTag { IP, BT };

// All possible connection types.
// L2CAP connections are not supported on Windows because of limitations with the Microsoft Bluetooth stack.
enum class ConnectionType { None, TCP, UDP, L2CAP, RFCOMM };

// IP versions.
enum class IPType { None, IPv4, IPv6 };

inline const char* getConnectionTypeName(ConnectionType type) {
    using enum ConnectionType;
    switch (type) {
        case TCP:
            return "TCP";
        case UDP:
            return "UDP";
        case L2CAP:
            return "L2CAP";
        case RFCOMM:
            return "RFCOMM";
        case None:
            return "None";
        default:
            return "Unknown connection type";
    }
}

inline const char* getIPTypeName(IPType type) {
    using enum IPType;
    switch (type) {
        case None:
            return "None";
        case IPv4:
            return "IPv4";
        case IPv6:
            return "IPv6";
        default:
            return "Unknown IP type";
    }
}
