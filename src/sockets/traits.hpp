// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_WINDOWS
// Winsock header
#include <WinSock2.h>
#elif OS_MACOS
#include <string>
#include <vector>

#include "plat_macos/bthandle.h"
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
#elif OS_MACOS
template <auto Tag>
struct SocketTraits {};

template <>
struct SocketTraits<SocketTag::IP> {
    using HandleType = int;
    static constexpr auto invalidHandle = -1;
};

template <>
struct SocketTraits<SocketTag::BT> {
#if __OBJC__
    using HandleType = BTHandle*;
#else
    // Objective-C types are not visible to C++ code, use a type of the same size as a filler
    // Class pointers take up 8 bytes: https://en.wikipedia.org/wiki/Tagged_pointer#Examples
    // On 64-bit systems, C++ pointers also take up 8 bytes
    using HandleType = int*;
#endif

    static constexpr auto invalidHandle = nullptr;
};
#else
template <auto Tag>
struct SocketTraits {
    using HandleType = int;
    static constexpr auto invalidHandle = -1;
};
#endif
