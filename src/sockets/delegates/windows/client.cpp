// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module sockets.delegates.client;
import external.platform;
import external.std;
import net.enums;
import net.netutils;
import os.async;
import os.async.platform;
import os.errcheck;
import utils.strings;

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

    static LPFN_CONNECTEX connectExPtr = nullptr;

    if (!connectExPtr) {
        // Load the ConnectEx() function
        GUID guid = WSAID_CONNECTEX;
        DWORD numBytes = 0;
        check(WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &connectExPtr, sizeof(connectExPtr),
            &numBytes, nullptr, nullptr));
    }

    check(connectExPtr(s, addr, static_cast<int>(len), nullptr, 0, nullptr, &result), checkTrue);
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
