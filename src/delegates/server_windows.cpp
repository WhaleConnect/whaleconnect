// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include <bit>
#include <functional>
#include <memory>

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

static void setupSocket(SOCKET s, const AddrInfoType* result) {
    Async::add(s);

    constexpr DWORD on = 1;
    call(FN(setsockopt, s, SOL_SOCKET, SO_REUSEADDR, std::bit_cast<char*>(&on), sizeof(on)));

    // Bind and listen
    call(FN(bind, s, result->ai_addr, static_cast<socklen_t>(result->ai_addrlen)));
    call(FN(listen, s, SOMAXCONN));
}

template <>
uint16_t Delegates::Server<SocketTag::IP>::startServer(uint16_t port) {
    auto addr = NetUtils::resolveAddr({ _type, "", "", port }, _traits.ip, AI_PASSIVE, true);

    bool isV4;

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

    sockaddr_storage localAddr;
    socklen_t localAddrLen = sizeof(localAddr);
    call(FN(getsockname, *_handle, std::bit_cast<sockaddr*>(&localAddr), &localAddrLen));

    return NetUtils::getPort(&localAddr, isV4);
}

template <auto Tag>
Task<AcceptResult> Delegates::Server<Tag>::accept() {
    constexpr bool isIP = Tag == SocketTag::IP;

    static LPFN_ACCEPTEX acceptExPtr = nullptr;

    if (!acceptExPtr) {
        // Load the AcceptEx() function
        GUID guid = WSAID_ACCEPTEX;
        DWORD numBytes = 0;
        call(FN(WSAIoctl, *_handle, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &acceptExPtr,
                sizeof(acceptExPtr), &numBytes, nullptr, nullptr));
    }

    // Buffer to hold local and remote addresses
    constexpr DWORD addrSize = sizeof(sockaddr_storage) + 16;
    std::vector<BYTE> buf(addrSize * 2);

    // Socket which is associated to the client upon accept
    constexpr int af = isIP ? AF_INET6 : AF_BTH;
    SocketHandle<SocketTag::IP> fd{ call(FN(socket, af, SOCK_STREAM, 0)) };
    Async::add(*fd);

    // Accept and update context on the client socket
    co_await Async::run([this, &fd, &buf](Async::CompletionResult& result) {
        call(FN(acceptExPtr, *_handle, *fd, buf.data(), 0, addrSize, addrSize, nullptr, &result), checkTrue);
    });

    call(FN(setsockopt, *fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, std::bit_cast<char*>(&*_handle), sizeof(*_handle)));

    static LPFN_GETACCEPTEXSOCKADDRS getAcceptExSockaddrsPtr = nullptr;

    if (!getAcceptExSockaddrsPtr) {
        // Load the GetAcceptExSockaddrs() function
        GUID guid = WSAID_GETACCEPTEXSOCKADDRS;
        DWORD numBytes = 0;
        call(FN(WSAIoctl, *_handle, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &getAcceptExSockaddrsPtr,
                sizeof(getAcceptExSockaddrsPtr), &numBytes, nullptr, nullptr));
    }

    sockaddr* localAddrPtr;
    sockaddr* remoteAddrPtr;
    int localAddrLen;
    int remoteAddrLen;

    getAcceptExSockaddrsPtr(buf.data(), 0, addrSize, addrSize, &localAddrPtr, &localAddrLen, &remoteAddrPtr,
                            &remoteAddrLen);

    if constexpr (isIP) {
        Device device = NetUtils::fromAddr(remoteAddrPtr, remoteAddrLen, ConnectionType::TCP);
        co_return { device, std::make_unique<IncomingSocket<Tag>>(std::move(fd)) };
    } else {
        co_return {};
    }
}

template Task<AcceptResult> Delegates::Server<SocketTag::IP>::accept();
template Task<AcceptResult> Delegates::Server<SocketTag::BT>::accept();
#endif
