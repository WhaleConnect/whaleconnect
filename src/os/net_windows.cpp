// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include "net_internal.hpp"

#include <functional>

#include <MSWSock.h>
#include <WinSock2.h>
#include <ws2bth.h>

#include "errcheck.hpp"
#include "error.hpp"
#include "net.hpp"
#include "socket.hpp"
#include "utils/strings.hpp"

void Net::init() {
    // Start Winsock on Windows
    WSADATA wsaData{};
    EXPECT_ZERO_RC(WSAStartup, MAKEWORD(2, 2), &wsaData); // MAKEWORD(2, 2) for Winsock 2.2
}

void Net::cleanup() {
    // Cleanup Winsock on Windows
    EXPECT_ZERO(WSACleanup);
}

void Net::Internal::startConnect(SOCKET s, const sockaddr* addr, socklen_t len, bool isDgram,
                                 Async::CompletionResult& result) {
    // Add the socket to the async queue
    Async::add(s);

    // Datagram sockets can be directly connected (ConnectEx() doesn't support them)
    if (isDgram) {
        EXPECT_ZERO(connect, s, addr, len);
        return;
    }

    // ConnectEx() requires the socket to be initially bound.
    // A sockaddr_storage can be used with all connection types, Internet and Bluetooth.
    sockaddr_storage addrBind{ .ss_family = addr->sa_family };

    // The bind() function will work with sockaddr_storage for any address family. However, with Bluetooth, it expects
    // the size parameter to be the size of a Bluetooth address structure. Unlike Internet-based sockets, it will not
    // accept a sockaddr_storage size.
    // This means the size must be spoofed with Bluetooth sockets.
    int addrSize = (addr->sa_family == AF_BTH) ? sizeof(SOCKADDR_BTH) : sizeof(sockaddr_storage);

    // Bind the socket
    EXPECT_ZERO(bind, s, reinterpret_cast<sockaddr*>(&addrBind), addrSize);

    // Load the ConnectEx() function
    LPFN_CONNECTEX connectExPtr = nullptr;

    GUID guid = WSAID_CONNECTEX;
    DWORD numBytes = 0;
    EXPECT_ZERO(WSAIoctl, s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &connectExPtr,
                sizeof(connectExPtr), &numBytes, nullptr, nullptr);

    // Call ConnectEx()
    EXPECT_TRUE(connectExPtr, s, addr, len, nullptr, 0, nullptr, &result);
}

void Net::Internal::finalizeConnect(SOCKET s, bool isDgram) {
    // Make the socket behave more like a regular socket connected with connect()
    if (!isDgram) EXPECT_ZERO(setsockopt, s, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0);
}

Task<Socket> Net::Internal::createClientSocketBT(const DeviceData& data) {
    // Only RFCOMM sockets are supported by the Microsoft Bluetooth stack on Windows
    if (data.type != Net::ConnectionType::RFCOMM)
        throw System::SystemError{ WSAEPFNOSUPPORT, System::ErrorType::System, "socket" };

    SOCKET fd = EXPECT_NONERROR(socket, AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    Socket ret{ fd };

    // Convert the MAC address from string form into integer form
    // This is done by removing all colons in the address string, then parsing the resultant string as an
    // integer in base-16 (which is how a MAC address is structured).
    BTH_ADDR btAddr = std::stoull(Strings::replaceAll(data.address, ":", ""), nullptr, 16);

    SOCKADDR_BTH sAddrBT{ .addressFamily = AF_BTH, .btAddr = btAddr, .port = data.port };
    auto addr = std::bit_cast<sockaddr*>(&sAddrBT);
    auto addrLen = static_cast<socklen_t>(sizeof(sAddrBT));

    co_await Async::run(std::bind_front(startConnect, fd, addr, addrLen, false));
    finalizeConnect(fd, false);

    co_return std::move(ret);
}
#endif
