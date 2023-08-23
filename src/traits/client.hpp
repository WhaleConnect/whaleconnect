// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_WINDOWS
// Winsock headers
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <netdb.h> // addrinfo/getaddrinfo() related identifiers
#endif

#include "net/device.hpp"
#include "net/enums.hpp"
#include "utils/handleptr.hpp"
#include "utils/task.hpp"

// Winsock-specific definitions and their Berkeley equivalents
#if !OS_WINDOWS
constexpr auto GetAddrInfo = getaddrinfo;
constexpr auto FreeAddrInfo = freeaddrinfo;
using ADDRINFOW = addrinfo;
#endif

namespace Traits {
    // Traits for client sockets.
    template <auto Tag>
    struct Client {
        Device device;
    };

    // Specific traits for IP client sockets.
    template <>
    struct Client<SocketTag::IP> {
        HandlePtr<ADDRINFOW, FreeAddrInfo> addr; // Address from getaddrinfo
        Device device;
    };
}
