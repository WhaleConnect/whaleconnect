// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sockets/delegates/client.hpp"

#include <functional>
#include <string>
#include <utility>

#include <WinSock2.h>
#include <MSWSock.h>
#include <ws2bth.h>

#include "net/enums.hpp"
#include "net/netutils.hpp"
#include "os/async.hpp"
#include "os/errcheck.hpp"
#include "utils/strings.hpp"

void startConnect(SOCKET s, sockaddr* addr, std::size_t len, Async::CompletionResult& result) {
    // ConnectEx() requires the socket to be initially bound.
    // A sockaddr_storage can be used with all connection types, Internet and Bluetooth.
    sockaddr_storage addrBind{ .ss_family = addr->sa_family };

    // The bind() function will work with sockaddr_storage for any address family. However, with Bluetooth, it expects
    // the size parameter to be the size of a Bluetooth address structure. Unlike Internet-based sockets, it will not
    // accept a sockaddr_storage size.
    // This means the size must be spoofed with Bluetooth sockets.
    int addrSize = addr->sa_family == AF_BTH ? sizeof(SOCKADDR_BTH) : sizeof(sockaddr_storage);

    // Bind the socket
    check(bind(s, reinterpret_cast<sockaddr*>(&addrBind), addrSize));
    Async::submit(Async::Connect{ { s, &result }, addr, static_cast<socklen_t>(len) });
}

void finalizeConnect(SOCKET s) {
    // Make the socket behave more like a regular socket connected with connect()
    check(setsockopt(s, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0));
}

template <>
Task<> Delegates::Client<SocketTag::IP>::connect(Device device) {
    auto addr = NetUtils::resolveAddr(device);

    co_await NetUtils::loopWithAddr(addr.get(), [this, type = device.type](const AddrInfoType* result) -> Task<> {
        handle.reset(check(socket(result->ai_family, result->ai_socktype, result->ai_protocol)));

        // Add the socket to the async queue
        Async::add(*handle);

        // Datagram sockets can be directly connected (ConnectEx doesn't support them)
        if (type == ConnectionType::UDP) {
            check(::connect(*handle, result->ai_addr, static_cast<int>(result->ai_addrlen)));
        } else {
            co_await Async::run(std::bind_front(startConnect, *handle, result->ai_addr, result->ai_addrlen));
            finalizeConnect(*handle);
        }
    });
}

template <>
Task<> Delegates::Client<SocketTag::BT>::connect(Device device) {
    // Only RFCOMM sockets are supported by the Microsoft Bluetooth stack on Windows
    if (device.type != ConnectionType::RFCOMM) std::unreachable();

    handle.reset(check(socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM)));
    Async::add(*handle);

    // Convert the MAC address from string form into integer form
    // This is done by removing all colons in the address string, then parsing the resultant string as an
    // integer in base-16 (which is how a MAC address is structured).
    BTH_ADDR btAddr = std::stoull(Strings::replaceAll(device.address, ":", ""), nullptr, 16);
    SOCKADDR_BTH sAddrBT{ .addressFamily = AF_BTH, .btAddr = btAddr, .port = device.port };

    co_await Async::run(std::bind_front(startConnect, *handle, reinterpret_cast<sockaddr*>(&sAddrBT), sizeof(sAddrBT)));
    finalizeConnect(*handle);
}
