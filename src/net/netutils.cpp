// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "netutils.hpp"

#include <stdexcept>

#include "os/errcheck.hpp"
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
