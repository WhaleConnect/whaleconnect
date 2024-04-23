// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <exception>
#include <type_traits>

#if OS_WINDOWS
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#endif

#include "device.hpp"
#include "enums.hpp"
#include "os/error.hpp"
#include "sockets/delegates/delegates.hpp"
#include "sockets/delegates/sockethandle.hpp"
#include "sockets/delegates/traits.hpp"
#include "utils/handleptr.hpp"
#include "utils/task.hpp"

// Winsock-specific definitions and their Berkeley equivalents
#if OS_WINDOWS
using AddrInfoType = ADDRINFOW;
using AddrInfoHandle = HandlePtr<AddrInfoType, FreeAddrInfo>;
#else
using AddrInfoType = addrinfo;
using AddrInfoHandle = HandlePtr<AddrInfoType, freeaddrinfo>;
#endif

namespace NetUtils {
    // Resolves an address with getaddrinfo.
    AddrInfoHandle resolveAddr(const Device& device, bool useDNS = true);

    // Loops through a getaddrinfo result.
    template <class Fn>
    requires std::same_as<std::invoke_result_t<Fn, AddrInfoType*>, Task<>>
    Task<> loopWithAddr(const AddrInfoType* addr, Fn fn) {
        std::exception_ptr lastException;
        for (auto result = addr; result; result = result->ai_next) {
            try {
                co_await fn(result);
                co_return;
            } catch (const System::SystemError& e) {
                lastException = std::current_exception();

                // Leave loop if operation was canceled
                if (e.isCanceled()) break;
            }
        }

        std::rethrow_exception(lastException);
    }

    // Loops through a getaddrinfo result.
    template <class Fn>
    void loopWithAddr(const AddrInfoType* addr, Fn fn) {
        std::exception_ptr lastException;
        for (auto result = addr; result; result = result->ai_next) {
            try {
                fn(result);
                return;
            } catch (const System::SystemError&) {
                lastException = std::current_exception();
            }
        }

        std::rethrow_exception(lastException);
    }

    // Returns address information with getnameinfo.
    Device fromAddr(const sockaddr* addr, socklen_t addrLen, ConnectionType type);

    // Returns the port from a sockaddr.
    std::uint16_t getPort(Traits::SocketHandleType<SocketTag::IP> handle, bool isV4);

    // Starts a server with the specified socket handle.
    ServerAddress startServer(const Device& serverInfo, Delegates::SocketHandle<SocketTag::IP>& handle);
}
