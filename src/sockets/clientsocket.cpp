// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "clientsocket.hpp"

#include "device.hpp"
#include "traits.hpp"

#include "os/errcheck.hpp"
#include "utils/out_ptr_compat.hpp"
#include "utils/strings.hpp"

template <>
std::unique_ptr<ClientSocket<SocketTag::IP>> createClientSocket(const Device& device) {
    bool isUDP = (device.type == ConnectionType::UDP);

    ADDRINFOW hints{
        .ai_family = AF_UNSPEC,
        .ai_socktype = isUDP ? SOCK_DGRAM : SOCK_STREAM,
        .ai_protocol = isUDP ? IPPROTO_UDP : IPPROTO_TCP,
    };

    // Wide encoding conversions for Windows
    Strings::SysStr addrWide = Strings::toSys(device.address);
    Strings::SysStr portWide = Strings::toSys(device.port);

    // Resolve and connect to the IP, getaddrinfo() and GetAddrInfoW() allow both IPv4 and IPv6 addresses
    ClientSocketTraits<SocketTag::IP> traits;
    call(FN(GetAddrInfo, addrWide.c_str(), portWide.c_str(), &hints, std2::out_ptr(traits._addr)), checkZero,
         useReturnCode, System::ErrorType::AddrInfo);

    // Initialize socket
    for (ADDRINFOW* addr = traits._addr.get(); addr; addr = addr->ai_next) {
        auto fd = socket(traits._addr->ai_family, traits._addr->ai_socktype, traits._addr->ai_protocol);

        if (fd != SocketTraits<SocketTag::IP>::invalidHandle)
            return std::make_unique<ClientSocket<SocketTag::IP>>(fd, device, std::move(traits));
    }

    throw System::SystemError{ APP_NO_IP, System::ErrorType::Application, "createClientSocket" };
}
