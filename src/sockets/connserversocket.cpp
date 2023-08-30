// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <bit>

#if OS_WINDOWS
#include <WinSock2.h>
#include <ws2ipdef.h>

#include "os/async_windows.hpp"
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include "connserversocket.hpp"

#include "net/enums.hpp"
#include "net/netutils.hpp"
#include "os/errcheck.hpp"

#if OS_WINDOWS
using OptType = DWORD;
using OptPtrType = char*;
#else
using OptType = int;
using OptPtrType = int*;
#endif

template <>
void ConnServerSocket<SocketTag::IP>::_init(uint16_t port, int backlog) {
    _handle.reset(call(FN(socket, AF_INET6, SOCK_STREAM, 0)));

#if OS_WINDOWS
    // Add the handle to IOCP
    Async::add(*_handle);
#endif

    // Enable dual-stack server and address reuse
    constexpr OptType off = 0;
    constexpr OptType on = 1;
    call(FN(setsockopt, *_handle, IPPROTO_IPV6, IPV6_V6ONLY, std::bit_cast<OptPtrType>(&off), sizeof(off)));
    call(FN(setsockopt, *_handle, SOL_SOCKET, SO_REUSEADDR, std::bit_cast<OptPtrType>(&on), sizeof(on)));

    auto addr = NetUtils::resolveAddr({ ConnectionType::TCP, "", "", port }, NetUtils::IPType::V6, AI_PASSIVE, true);

    // Bind and listen
    call(FN(bind, *_handle, addr->ai_addr, addr->ai_addrlen));
    call(FN(listen, *_handle, backlog));
}
