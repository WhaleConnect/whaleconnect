// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include <bit>
#include <exception>
#include <functional>

#include <WinSock2.h>
#include <MSWSock.h>
#include <ws2bth.h>

#include "client.hpp"

#include "net/netutils.hpp"
#include "os/async_windows.hpp"
#include "os/errcheck.hpp"
#include "utils/strings.hpp"

static void startConnect(SOCKET s, sockaddr* addr, size_t len, Async::CompletionResult& result) {
    // ConnectEx() requires the socket to be initially bound.
    // A sockaddr_storage can be used with all connection types, Internet and Bluetooth.
    sockaddr_storage addrBind{ .ss_family = addr->sa_family };

    // The bind() function will work with sockaddr_storage for any address family. However, with Bluetooth, it expects
    // the size parameter to be the size of a Bluetooth address structure. Unlike Internet-based sockets, it will not
    // accept a sockaddr_storage size.
    // This means the size must be spoofed with Bluetooth sockets.
    int addrSize = (addr->sa_family == AF_BTH) ? sizeof(SOCKADDR_BTH) : sizeof(sockaddr_storage);

    // Bind the socket
    call(FN(bind, s, reinterpret_cast<sockaddr*>(&addrBind), addrSize));

    static LPFN_CONNECTEX connectExPtr = nullptr;

    if (!connectExPtr) {
        // Load the ConnectEx() function
        GUID guid = WSAID_CONNECTEX;
        DWORD numBytes = 0;
        call(FN(WSAIoctl, s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &connectExPtr,
                sizeof(connectExPtr), &numBytes, nullptr, nullptr));
    }

    call(FN(connectExPtr, s, addr, static_cast<int>(len), nullptr, 0, nullptr, &result), checkTrue);
}

static void finalizeConnect(SOCKET s) {
    // Make the socket behave more like a regular socket connected with connect()
    call(FN(setsockopt, s, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0));
}

template <>
Task<> Delegates::Client<SocketTag::IP>::connect() {
    auto addr = NetUtils::resolveAddr(_device, IPType::None);

    co_await NetUtils::loopWithAddr(addr.get(), [this](const AddrInfoType* result) -> Task<> {
        _handle.reset(call(FN(socket, result->ai_family, result->ai_socktype, result->ai_protocol)));

        // Add the socket to the async queue
        Async::add(*_handle);

        // Datagram sockets can be directly connected (ConnectEx doesn't support them)
        if (_device.type == ConnectionType::UDP) {
            call(FN(::connect, *_handle, result->ai_addr, static_cast<int>(result->ai_addrlen)));
        } else {
            co_await Async::run(std::bind_front(startConnect, *_handle, result->ai_addr, result->ai_addrlen));
            finalizeConnect(*_handle);
        }
    });
}

template <>
Task<> Delegates::Client<SocketTag::BT>::connect() {
    // Only RFCOMM sockets are supported by the Microsoft Bluetooth stack on Windows
    if (_device.type != ConnectionType::RFCOMM)
        throw System::SystemError{ WSAEPFNOSUPPORT, System::ErrorType::System, "socket" };

    _handle.reset(call(FN(socket, AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM)));

    // Convert the MAC address from string form into integer form
    // This is done by removing all colons in the address string, then parsing the resultant string as an
    // integer in base-16 (which is how a MAC address is structured).
    BTH_ADDR btAddr = std::stoull(Strings::replaceAll(_device.address, ":", ""), nullptr, 16);
    SOCKADDR_BTH sAddrBT{ .addressFamily = AF_BTH, .btAddr = btAddr, .port = _device.port };

    co_await Async::run(std::bind_front(startConnect, *_handle, std::bit_cast<sockaddr*>(&sAddrBT), sizeof(sAddrBT)));
    finalizeConnect(*_handle);
}
#endif
