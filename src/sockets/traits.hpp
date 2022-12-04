// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_WINDOWS
// Winsock header
#include <WinSock2.h>
#elif OS_APPLE
#include <string>
#include <vector>

#include <IOBluetooth/IOBluetoothUserLib.h>
#endif

// Enumeration to determine socket types at compile time.
enum class SocketTag { IP, BT };

// All possible connection types.
// L2CAP connections are not supported on Windows because of limitations with the Microsoft Bluetooth stack.
enum class ConnectionType { TCP, UDP, L2CAPSeqPacket, L2CAPStream, L2CAPDgram, RFCOMM, None };

#if OS_WINDOWS
template <auto Tag>
struct SocketTraits {
    using HandleType = SOCKET;
    static constexpr auto invalidHandle = INVALID_SOCKET;
};
#elif OS_APPLE
template <auto Tag>
struct SocketTraits {};

template <>
struct SocketTraits<SocketTag::IP> {
    using HandleType = int;
    static constexpr auto invalidHandle = -1;
};

template <>
struct SocketTraits<SocketTag::BT> {
    struct HandleType {
        IOBluetoothObjectID handle;
        ConnectionType type;

        IOBluetoothObjectID operator*() const { return handle; }
    };

    static constexpr auto invalidHandle = HandleType{ 0, ConnectionType::None };
};
#else
template <auto Tag>
struct SocketTraits {
    using HandleType = int;
    static constexpr auto invalidHandle = -1;
};
#endif
