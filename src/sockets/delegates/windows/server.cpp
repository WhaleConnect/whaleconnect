// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <coroutine> // IWYU pragma: keep
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <WinSock2.h>
#include <bluetoothapis.h>
#include <MSWSock.h>
#include <ws2bth.h>
#include <ws2ipdef.h>
#include <WS2tcpip.h>
#include <ztd/out_ptr.hpp>

module sockets.delegates.server;
import net.netutils;
import os.async;
import os.async.platform;
import os.errcheck;
import sockets.incomingsocket;
import sockets.socket; // Prevents "LNK1227: conflicting weak extern definition" (MSVC)
import utils.handleptr;
import utils.strings;
import utils.task;

// Buffer to hold local and remote addresses
constexpr DWORD addrSize = sizeof(sockaddr_storage) + 16;
using AcceptExBuf = std::vector<BYTE>;

Task<std::pair<sockaddr*, int>> startAccept(SOCKET s, AcceptExBuf& buf, SOCKET clientSocket) {
    static LPFN_ACCEPTEX acceptExPtr = nullptr;

    if (!acceptExPtr) {
        // Load the AcceptEx() function
        GUID guid = WSAID_ACCEPTEX;
        DWORD numBytes = 0;
        check(WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &acceptExPtr, sizeof(acceptExPtr),
            &numBytes, nullptr, nullptr));
    }

    // Accept and update context on the client socket
    co_await Async::run([s, clientSocket, &buf](Async::CompletionResult& result) {
        check(acceptExPtr(s, clientSocket, buf.data(), 0, addrSize, addrSize, nullptr, &result), checkTrue);
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
Task<DgramRecvResult> Delegates::Server<SocketTag::IP>::recvFrom(size_t size) {
    sockaddr_storage from;
    auto fromPtr = reinterpret_cast<sockaddr*>(&from);
    socklen_t fromLen = sizeof(from);

    std::string data(size, 0);
    auto recvResult = co_await Async::run([this, &data, fromPtr, &fromLen](Async::CompletionResult& result) {
        DWORD flags = 0;
        WSABUF buf{ static_cast<ULONG>(data.size()), data.data() };
        check(WSARecvFrom(*handle, &buf, 1, nullptr, &flags, fromPtr, &fromLen, &result, nullptr));
    });

    data.resize(recvResult.res);

    co_return { NetUtils::fromAddr(fromPtr, fromLen, ConnectionType::UDP), data };
}

template <>
Task<> Delegates::Server<SocketTag::IP>::sendTo(Device device, std::string data) {
    auto addr = NetUtils::resolveAddr(device, false);

    co_await NetUtils::loopWithAddr(addr.get(), [this, &data](const AddrInfoType* resolveRes) -> Task<> {
        co_await Async::run([this, resolveRes, &data](Async::CompletionResult& result) {
            WSABUF buf{ static_cast<ULONG>(data.size()), data.data() };
            check(WSASendTo(*handle, &buf, 1, nullptr, 0, resolveRes->ai_addr, static_cast<int>(resolveRes->ai_addrlen),
                &result, nullptr));
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
    return { static_cast<uint16_t>(serverAddr.port), IPType::None };
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
    check(WSAAddressToString(remoteAddrPtr, remoteAddrLen, nullptr, clientAddrW.data(), &addrLen));

    std::string clientAddr = Strings::fromSys(clientAddrW).substr(1, 17);

    // Get device name
    BLUETOOTH_DEVICE_INFO deviceInfo{
        .dwSize = sizeof(BLUETOOTH_DEVICE_INFO),
        .Address = std::stoull(Strings::replaceAll(clientAddr, ":", ""), nullptr, 16)
    };

    check(BluetoothGetDeviceInfo(nullptr, &deviceInfo), checkZero, useReturnCode);

    Device device{ ConnectionType::RFCOMM, Strings::fromSys(deviceInfo.szName), clientAddr, static_cast<uint16_t>(clientPtr->port) };
    co_return { device, std::make_unique<IncomingSocket<SocketTag::BT>>(std::move(fd)) };
}
