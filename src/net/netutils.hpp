// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_WINDOWS
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#endif

#include "device.hpp"
#include "enums.hpp"

#include "sockets/clientsocket.hpp"
#include "utils/handleptr.hpp"

// Winsock-specific definitions and their Berkeley equivalents
#if OS_WINDOWS
using AddrInfoType = ADDRINFOW;
using AddrInfoHandle = HandlePtr<AddrInfoType, FreeAddrInfo>;
#else
using AddrInfoType = addrinfo;
using AddrInfoHandle = HandlePtr<AddrInfoType, freeaddrinfo>;
#endif

namespace NetUtils {
    // Resolves an address with getaddrinfo.
    AddrInfoHandle resolveAddr(const Device& device, IPType ipType, int flags = 0, bool useLoopback = false);

    // Returns address information with getnameinfo.
    Device fromAddr(sockaddr* addr, socklen_t addrLen, ConnectionType type);
}
