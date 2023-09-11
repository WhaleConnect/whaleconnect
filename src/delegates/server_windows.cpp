// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include <bit>
#include <functional>
#include <memory>
#include <utility>

#include <WinSock2.h>
#include <MSWSock.h>
#include <ws2ipdef.h>
#include <WS2tcpip.h>

#include "server.hpp"

#include "net/netutils.hpp"
#include "os/async_windows.hpp"
#include "os/errcheck.hpp"
#include "sockets/incomingsocket.hpp"
#include "utils/strings.hpp"
#include "utils/task.hpp"

// Buffer to hold local and remote addresses
constexpr DWORD addrSize = sizeof(sockaddr_storage) + 16;
using AcceptExBuf = std::vector<BYTE>;

static void setupSocket(SOCKET s, const AddrInfoType* result) {
    Async::add(s);

    constexpr DWORD on = 1;
    call(FN(setsockopt, s, SOL_SOCKET, SO_REUSEADDR, std::bit_cast<char*>(&on), sizeof(on)));

    // Bind and listen
    call(FN(bind, s, result->ai_addr, static_cast<socklen_t>(result->ai_addrlen)));
    call(FN(listen, s, SOMAXCONN));
}

static Task<std::pair<sockaddr*, int>> startAccept(SOCKET s, AcceptExBuf& buf, SOCKET clientSocket) {
    static LPFN_ACCEPTEX acceptExPtr = nullptr;

    if (!acceptExPtr) {
        // Load the AcceptEx() function
        GUID guid = WSAID_ACCEPTEX;
        DWORD numBytes = 0;
        call(FN(WSAIoctl, s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &acceptExPtr, sizeof(acceptExPtr),
                &numBytes, nullptr, nullptr));
    }

    // Accept and update context on the client socket
    co_await Async::run([s, clientSocket, &buf](Async::CompletionResult& result) {
        call(FN(acceptExPtr, s, clientSocket, buf.data(), 0, addrSize, addrSize, nullptr, &result), checkTrue);
    });

    call(FN(setsockopt, clientSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, std::bit_cast<char*>(&s), sizeof(s)));
    Async::add(clientSocket);

    static LPFN_GETACCEPTEXSOCKADDRS getAcceptExSockaddrsPtr = nullptr;

    if (!getAcceptExSockaddrsPtr) {
        // Load the GetAcceptExSockaddrs() function
        GUID guid = WSAID_GETACCEPTEXSOCKADDRS;
        DWORD numBytes = 0;
        call(FN(WSAIoctl, s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &getAcceptExSockaddrsPtr,
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
ServerAddress Delegates::Server<SocketTag::IP>::startServer(uint16_t port) {
    auto addr = NetUtils::resolveAddr({ _type, "", "", port }, _traits.ip, AI_PASSIVE, true);
    bool isV4 = false;

    NetUtils::loopWithAddr(addr.get(), [this, &isV4](const AddrInfoType* result) {
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

        _handle.reset(call(FN(socket, result->ai_family, result->ai_socktype, result->ai_protocol)));
        setupSocket(*_handle, result);
    });

    _traits.ip = isV4 ? IPType::IPv4 : IPType::IPv6;
    return { NetUtils::getPort(*_handle, isV4), _traits.ip };
}

template <>
Task<AcceptResult> Delegates::Server<SocketTag::IP>::accept() {
    // Socket which is associated to the client upon accept
    int af = (_traits.ip == IPType::IPv4) ? AF_INET : AF_INET6;
    SocketHandle<SocketTag::IP> fd{ call(FN(socket, af, SOCK_STREAM, 0)) };

    std::vector<BYTE> buf(addrSize * 2);
    auto [remoteAddrPtr, remoteAddrLen] = co_await startAccept(*_handle, buf, *fd);

    Device device = NetUtils::fromAddr(remoteAddrPtr, remoteAddrLen, ConnectionType::TCP);
    co_return { device, std::make_unique<IncomingSocket<SocketTag::IP>>(std::move(fd)) };
}

template <>
Task<AcceptResult> Delegates::Server<SocketTag::BT>::accept() {
    // TODO
    co_return {};
}
#endif
