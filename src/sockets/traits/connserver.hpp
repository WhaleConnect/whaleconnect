// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_WINDOWS
#include <WinSock2.h>
#elif OS_LINUX
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <netinet/in.h>
#endif

#include "sockets/enums.hpp"

namespace Traits {
    template <auto Tag>
    struct ConnServer {};

    template <>
    struct ConnServer<SocketTag::IP> {
        sockaddr_in6 addr;
        unsigned int addrLen;
    };

    template <>
    struct ConnServer<SocketTag::BT> {
#if OS_LINUX
        union {
            sockaddr_l2 addrL2;
            sockaddr_rc addrRC;
        } addr;
#endif
        unsigned int addrLen;
    };
}
