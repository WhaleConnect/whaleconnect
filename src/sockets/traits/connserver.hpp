// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_WINDOWS
#include <WinSock2.h>
#else
#include <sys/socket.h>
#endif

namespace Traits {
    template <auto Tag>
    struct ConnServer {
        sockaddr_storage addr;
        size_t addrLen;
    };
}
