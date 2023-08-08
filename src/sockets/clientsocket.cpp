// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "clientsocket.hpp"

#include "os/errcheck.hpp"
#include "os/error.hpp"
#include "traits/client.hpp"
#include "utils/out_ptr_compat.hpp"
#include "utils/strings.hpp"

template <>
void ClientSocketIP::_init() {
    const auto& device = _traits.device;
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
    call(FN(GetAddrInfo, addrWide.c_str(), portWide.c_str(), &hints, std2::out_ptr(_traits.addr)), checkZero,
         useReturnCode, System::ErrorType::AddrInfo);

    // Initialize socket
    for (ADDRINFOW* addr = _traits.addr.get(); addr; addr = addr->ai_next) {
        auto fd = socket(_traits.addr->ai_family, _traits.addr->ai_socktype, _traits.addr->ai_protocol);

        if (fd != Traits::invalidSocketHandle<SocketTag::IP>()) {
            _handle = fd;
            return;
        }
    }

    throw System::SystemError{ APP_NO_IP, System::ErrorType::Application, "ClientSocket" };
}
