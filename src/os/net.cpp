// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "net.hpp"
#include "net_internal.hpp"

#include <functional> // std::bind_front()

#if OS_WINDOWS
// Winsock headers
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <netdb.h>      // addrinfo/getaddrinfo() related identifiers
#include <sys/socket.h> // Socket definitions

// Winsock-specific definitions and their Berkeley equivalents
constexpr auto GetAddrInfo = getaddrinfo;
constexpr auto FreeAddrInfo = freeaddrinfo;
using ADDRINFOW = addrinfo;
#endif

#include "async.hpp"
#include "errcheck.hpp"
#include "error.hpp"
#include "socket.hpp"
#include "utils/handleptr.hpp"
#include "utils/out_ptr_compat.hpp"
#include "utils/strings.hpp"

static Task<Socket> createClientSocketIP(const Net::DeviceData& data) {
    using enum Net::ConnectionType;

    bool isUDP = (data.type == UDP);

    ADDRINFOW hints{
        .ai_flags = AI_NUMERICHOST,
        .ai_family = AF_UNSPEC,
        .ai_socktype = isUDP ? SOCK_DGRAM : SOCK_STREAM,
        .ai_protocol = isUDP ? IPPROTO_UDP : IPPROTO_TCP,
    };

    // Wide encoding conversions for Windows
    Strings::SysStr addrWide = Strings::toSys(data.address);
    Strings::SysStr portWide = Strings::toSys(data.port);

    // Resolve and connect to the IP, getaddrinfo() and GetAddrInfoW() allow both IPv4 and IPv6 addresses
    HandlePtr<ADDRINFOW, FreeAddrInfo> addr;
    EXPECT_ZERO_RC_TYPE(GetAddrInfo, System::ErrorType::AddrInfo, addrWide.c_str(), portWide.c_str(), &hints,
                        std2::out_ptr(addr));

    // Initialize socket
    SOCKET fd = EXPECT_NONERROR(socket, addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    Socket ret{ fd };

    // Connect to the server
    auto addrLen = static_cast<socklen_t>(addr->ai_addrlen);
    co_await Async::run(std::bind_front(Net::Internal::startConnect, fd, addr->ai_addr, addrLen, isUDP));
    Net::Internal::finalizeConnect(fd, isUDP);

    co_return std::move(ret);
}

Task<Socket> Net::createClientSocket(const DeviceData& data) {
    if (connectionTypeIsIP(data.type)) co_return co_await createClientSocketIP(data);

    if (connectionTypeIsBT(data.type)) co_return co_await Internal::createClientSocketBT(data);

    // None type
    throw std::invalid_argument{ "None type specified in socket creation" };
}
