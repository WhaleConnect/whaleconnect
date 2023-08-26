// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include <WinSock2.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include "connserversocket.hpp"

#include "net/enums.hpp"
#include "os/errcheck.hpp"

template <>
void ConnServerSocket<SocketTag::IP>::_init(uint16_t port, int backlog) {
    _handle = call(FN(socket, AF_INET6, SOCK_STREAM, 0));

    // Enable dual-stack server and address reuse
    constexpr int off = 0;
    constexpr int on = 1;
    call(FN(setsockopt, _handle, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off)));
    call(FN(setsockopt, _handle, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)));

    sockaddr_in6 addr{
        .sin6_family = AF_INET6,
        .sin6_port = htons(port),
        .sin6_addr = in6addr_any,
    };
    unsigned int addrLen = sizeof(addr);

    // Bind and listen
    call(FN(bind, _handle, std::bit_cast<sockaddr*>(&addr), addrLen));
    call(FN(listen, _handle, backlog));
}
