// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "netutils.hpp"

#include <bit>
#include <stdexcept>

#include "enums.hpp"

#include "os/errcheck.hpp"
#include "traits/sockethandle.hpp"
#include "utils/out_ptr_compat.hpp"
#include "utils/strings.hpp"

#if !OS_WINDOWS
constexpr auto GetAddrInfo = getaddrinfo;
constexpr auto GetNameInfo = getnameinfo;
#endif

AddrInfoHandle NetUtils::resolveAddr(const Device& device, IPType ipType, int flags, bool useLoopback) {
    bool isUDP = device.type == ConnectionType::UDP;
    int family;

    using enum IPType;
    switch (ipType) {
        case None:
            family = AF_UNSPEC;
            break;
        case V4:
            family = AF_INET;
            break;
        case V6:
            family = AF_INET6;
            break;
        default:
            throw std::invalid_argument{ "Invalid IP type" };
    }

    AddrInfoType hints{
        .ai_flags = flags,
        .ai_family = family,
        .ai_socktype = isUDP ? SOCK_DGRAM : SOCK_STREAM,
        .ai_protocol = isUDP ? IPPROTO_UDP : IPPROTO_TCP,
    };

    // Wide encoding conversions for Windows
    Strings::SysStr addrWide = Strings::toSys(device.address);
    Strings::SysStr portWide = Strings::toSys(device.port);

    // Resolve the IP
    AddrInfoHandle ret;
    call(FN(GetAddrInfo, useLoopback ? nullptr : addrWide.c_str(), portWide.c_str(), &hints, std2::out_ptr(ret)),
         checkZero, useReturnCode, System::ErrorType::AddrInfo);

    return ret;
}

Device NetUtils::fromAddr(sockaddr* addr, socklen_t addrLen, ConnectionType type) {
    constexpr auto nullChar = Strings::SysStr::value_type{};

    Strings::SysStr ipStr(NI_MAXHOST, nullChar);
    Strings::SysStr portStr(NI_MAXSERV, nullChar);

    auto ipLen = static_cast<socklen_t>(ipStr.size());
    auto portLen = static_cast<socklen_t>(portStr.size());

    call(FN(GetNameInfo, addr, addrLen, ipStr.data(), ipLen, portStr.data(), portLen, NI_NUMERICHOST | NI_NUMERICSERV),
         checkZero, useReturnCode, System::ErrorType::AddrInfo);

    // Process returned strings
    Strings::stripNull(ipStr);
    std::string ip = Strings::fromSys(ipStr);
    auto port = static_cast<uint16_t>(std::stoi(Strings::fromSys(portStr)));

    return { type, "", ip, port };
}

uint16_t NetUtils::getPort(Traits::SocketHandleType<SocketTag::IP> handle, bool isV4) {
    sockaddr_storage addr;
    socklen_t localAddrLen = sizeof(addr);
    call(FN(getsockname, handle, std::bit_cast<sockaddr*>(&addr), &localAddrLen));

    uint16_t port
        = isV4 ? std::bit_cast<sockaddr_in*>(&addr)->sin_port : std::bit_cast<sockaddr_in6*>(&addr)->sin6_port;
    return ntohs(port);
}
