// Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sockets/delegates/server.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <WinSock2.h>
#include <bluetoothapis.h>
#include <MSWSock.h>
#include <ws2bth.h>
#include <ztd/out_ptr.hpp>

#include "net/netutils.hpp"
#include "os/async.hpp"
#include "os/errcheck.hpp"
#include "sockets/incomingsocket.hpp"
#include "utils/strings.hpp"
#include "utils/task.hpp"

// Buffer to hold local and remote addresses
constexpr DWORD addrSize = sizeof(sockaddr_storage) + 16;
using AcceptExBuf = std::vector<BYTE>;

Task<std::pair<sockaddr*, int>> startAccept(SOCKET s, AcceptExBuf& buf, SOCKET clientSocket) {
    // Accept and update context on the client socket
    co_await Async::run([s, clientSocket, &buf](Async::CompletionResult& result) {
        Async::submit(Async::Accept{ { s, &result }, clientSocket, buf.data() });
    });

    check(setsockopt(clientSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, reinterpret_cast<char*>(&s), sizeof(s)));
    Async::add(clientSocket);

    static LPFN_GETACCEPTEXSOCKADDRS getAcceptExSockaddrsPtr = nullptr;
    if (!getAcceptExSockaddrsPtr) {
        // Load the GetAcceptExSockaddrs() function
        GUID guid = WSAID_GETACCEPTEXSOCKADDRS;
        DWORD numBytes = 0;
        check(WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &getAcceptExSockaddrsPtr,
            sizeof(getAcceptExSockaddrsPtr), &numBytes, nullptr, nullptr));
    }

    sockaddr* localAddrPtr;
    sockaddr* remoteAddrPtr;
    int localAddrLen;
    int remoteAddrLen;

    // Get sockaddr pointers
    // Since the buffer is passed as a reference, the pointers will not become dangling when returned.
    getAcceptExSockaddrsPtr(buf.data(), 0, addrSize, addrSize, &localAddrPtr, &localAddrLen, &remoteAddrPtr,
        &remoteAddrLen);

    co_return { remoteAddrPtr, remoteAddrLen };
}

template <>
ServerAddress Delegates::Server<SocketTag::IP>::startServer(const Device& serverInfo) {
    ServerAddress result = NetUtils::startServer(serverInfo, handle);

    Async::add(*handle);
    traits.ip = result.ipType;

    return result;
}

template <>
Task<AcceptResult> Delegates::Server<SocketTag::IP>::accept() {
    // Socket which is associated to the client upon accept
    int af = traits.ip == IPType::IPv4 ? AF_INET : AF_INET6;
    SocketHandle<SocketTag::IP> fd{ check(socket(af, SOCK_STREAM, 0)) };

    std::vector<BYTE> buf(addrSize * 2);
    auto [remoteAddrPtr, remoteAddrLen] = co_await startAccept(*handle, buf, *fd);

    Device device = NetUtils::fromAddr(remoteAddrPtr, remoteAddrLen, ConnectionType::TCP);
    co_return { device, std::make_unique<IncomingSocket<SocketTag::IP>>(std::move(fd)) };
}

template <>
Task<DgramRecvResult> Delegates::Server<SocketTag::IP>::recvFrom(std::size_t size) {
    sockaddr_storage from;
    auto fromPtr = reinterpret_cast<sockaddr*>(&from);
    socklen_t fromLen = sizeof(from);

    std::string data(size, 0);
    auto recvResult = co_await Async::run([this, &data, fromPtr, &fromLen](Async::CompletionResult& result) {
        Async::submit(Async::ReceiveFrom{ { *handle, &result }, data, fromPtr, &fromLen });
    });

    data.resize(recvResult.res);

    co_return { NetUtils::fromAddr(fromPtr, fromLen, ConnectionType::UDP), data };
}

template <>
Task<> Delegates::Server<SocketTag::IP>::sendTo(Device device, std::string data) {
    auto addr = NetUtils::resolveAddr(device, false);

    co_await NetUtils::loopWithAddr(addr.get(), [this, &data](const AddrInfoType* resolveRes) -> Task<> {
        co_await Async::run([this, resolveRes, &data](Async::CompletionResult& result) {
            Async::submit(Async::SendTo{ { *handle, &result }, data, resolveRes->ai_addr,
                static_cast<socklen_t>(resolveRes->ai_addrlen) });
        });
    });
}

template <>
ServerAddress Delegates::Server<SocketTag::BT>::startServer(const Device& serverInfo) {
    handle.reset(check(socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM)));

    // Treat port 0 as "any port" (uses its own separate constant)
    ULONG port = serverInfo.port == 0 ? BT_PORT_ANY : serverInfo.port;
    SOCKADDR_BTH addr{ .addressFamily = AF_BTH, .btAddr = 0, .port = port };
    check(bind(*handle, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));
    check(listen(*handle, SOMAXCONN));

    SOCKADDR_BTH serverAddr;
    int serverAddrLen = sizeof(serverAddr);
    check(getsockname(*handle, reinterpret_cast<sockaddr*>(&serverAddr), &serverAddrLen));

    Async::add(*handle);
    return { static_cast<std::uint16_t>(serverAddr.port), IPType::None };
}

template <>
Task<AcceptResult> Delegates::Server<SocketTag::BT>::accept() {
    SocketHandle<SocketTag::BT> fd{ check(socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM)) };

    std::vector<BYTE> buf(addrSize * 2);
    auto [remoteAddrPtr, remoteAddrLen] = co_await startAccept(*handle, buf, *fd);

    //   | The WSAAddressToString function currently returns WSAEFAULT for Bluetooth Device Addresses unless the buffer
    //   | and the specified lpdwAddressStringLength value are set to a character length of 40.
    // https://learn.microsoft.com/en-us/windows/win32/bluetooth/bluetooth-and-wsaaddresstostring
    auto clientPtr = reinterpret_cast<SOCKADDR_BTH*>(remoteAddrPtr);
    std::wstring clientAddrW(40, L'\0');
    auto addrLen = static_cast<DWORD>(clientAddrW.size());
    check(WSAAddressToStringW(remoteAddrPtr, remoteAddrLen, nullptr, clientAddrW.data(), &addrLen));

    std::string clientAddr = Strings::fromSys(clientAddrW).substr(1, 17);

    // Get device name
    BLUETOOTH_DEVICE_INFO_STRUCT deviceInfo{ .dwSize = sizeof(BLUETOOTH_DEVICE_INFO_STRUCT),
        .Address = { std::stoull(Strings::replaceAll(clientAddr, ":", ""), nullptr, 16) } };

    check(BluetoothGetDeviceInfo(nullptr, &deviceInfo), checkZero, useReturnCode);

    Device device{ ConnectionType::RFCOMM, Strings::fromSys(deviceInfo.szName), clientAddr,
        static_cast<std::uint16_t>(clientPtr->port) };
    co_return { device, std::make_unique<IncomingSocket<SocketTag::BT>>(std::move(fd)) };
}
