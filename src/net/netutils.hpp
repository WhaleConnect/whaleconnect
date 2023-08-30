// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_WINDOWS
// Winsock headers
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
// addrinfo/getaddrinfo() related identifiers
#include <netdb.h>
#endif

#include "device.hpp"

#include "sockets/clientsocket.hpp"
#include "utils/handleptr.hpp"

// Winsock-specific definitions and their Berkeley equivalents
#if OS_WINDOWS
using AddrInfoType = ADDRINFOW;
#else
constexpr auto GetAddrInfo = getaddrinfo;
constexpr auto FreeAddrInfo = freeaddrinfo;
using AddrInfoType = addrinfo;
#endif

using AddrInfoHandle = HandlePtr<AddrInfoType, FreeAddrInfo>;

namespace NetUtils {
    enum class IPType { None, V4, V6 };

    // Resolves an address with getaddrinfo.
    AddrInfoHandle resolveAddr(const Device& device, IPType ipType, int flags = 0, bool useLoopback = false);
}
