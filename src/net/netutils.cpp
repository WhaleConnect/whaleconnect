// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "netutils.hpp"

#include <ztd/out_ptr.hpp>

#include "enums.hpp"
#include "os/errcheck.hpp"
#include "utils/strings.hpp"
#include "utils/uuids.hpp"

#if !OS_WINDOWS
constexpr auto GetAddrInfoW = getaddrinfo;
constexpr auto GetNameInfoW = getnameinfo;
#endif

AddrInfoHandle NetUtils::resolveAddr(const Device& device, bool useDNS) {
    bool isUDP = device.type == ConnectionType::UDP;
    AddrInfoType hints{
        .ai_flags = useDNS ? 0 : AI_NUMERICHOST,
        .ai_family = AF_UNSPEC,
        .ai_socktype = isUDP ? SOCK_DGRAM : SOCK_STREAM,
        .ai_protocol = isUDP ? IPPROTO_UDP : IPPROTO_TCP,
    };

    // Wide encoding conversions for Windows
    Strings::SysStr addrWide = Strings::toSys(device.address);
    Strings::SysStr portWide = Strings::toSys(device.port);

    // Resolve the IP
    AddrInfoHandle ret;
    check(GetAddrInfoW(addrWide.c_str(), portWide.c_str(), &hints, ztd::out_ptr::out_ptr(ret)), checkZero,
        useReturnCode, System::ErrorType::AddrInfo);

    return ret;
}

Device NetUtils::fromAddr(const sockaddr* addr, socklen_t addrLen, ConnectionType type) {
    constexpr auto nullChar = Strings::SysStr::value_type{};

    Strings::SysStr ipStr(NI_MAXHOST, nullChar);
    Strings::SysStr portStr(NI_MAXSERV, nullChar);

    auto ipLen = static_cast<socklen_t>(ipStr.size());
    auto portLen = static_cast<socklen_t>(portStr.size());

    check(GetNameInfoW(addr, addrLen, ipStr.data(), ipLen, portStr.data(), portLen, NI_NUMERICHOST | NI_NUMERICSERV),
        checkZero, useReturnCode, System::ErrorType::AddrInfo);

    // Process returned strings
    Strings::stripNull(ipStr);
    std::string ip = Strings::fromSys(ipStr);
    auto port = static_cast<std::uint16_t>(std::stoi(Strings::fromSys(portStr)));

    return { type, "", ip, port };
}

std::uint16_t NetUtils::getPort(Traits::SocketHandleType<SocketTag::IP> handle, bool isV4) {
    sockaddr_storage addr;
    socklen_t localAddrLen = sizeof(addr);
    check(getsockname(handle, reinterpret_cast<sockaddr*>(&addr), &localAddrLen));

    std::uint16_t port
        = isV4 ? reinterpret_cast<sockaddr_in*>(&addr)->sin_port : reinterpret_cast<sockaddr_in6*>(&addr)->sin6_port;
    return UUIDs::byteSwap(port);
}

ServerAddress NetUtils::startServer(const Device& serverInfo, Delegates::SocketHandle<SocketTag::IP>& handle) {
    auto resolved = resolveAddr(serverInfo);
    bool isTCP = serverInfo.type == ConnectionType::TCP;
    bool isV4 = false;

    NetUtils::loopWithAddr(resolved.get(), [&handle, &isV4, isTCP](const AddrInfoType* result) {
        // Only AF_INET/AF_INET6 are supported
        switch (result->ai_family) {
            case AF_INET:
                isV4 = true;
                break;
            case AF_INET6:
                isV4 = false;
                break;
            default:
                return;
        }

        handle.reset(check(socket(result->ai_family, result->ai_socktype, result->ai_protocol)));

        // Bind and listen
        check(bind(*handle, result->ai_addr, static_cast<socklen_t>(result->ai_addrlen)));
        if (isTCP) check(listen(*handle, SOMAXCONN));
    });

    return { getPort(*handle, isV4), isV4 ? IPType::IPv4 : IPType::IPv6 };
}
