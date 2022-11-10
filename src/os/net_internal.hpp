// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_WINDOWS
using socklen_t = int;
#else
#include <sys/socket.h>
#endif

#include "async.hpp"
#include "net.hpp"
#include "socket.hpp"
#include "utils/task.hpp"

struct sockaddr;

namespace Net::Internal {
    void startConnect(SOCKET s, const sockaddr* addr, socklen_t len, bool isDgram, Async::CompletionResult& result);

    void finalizeConnect(SOCKET s, bool isDgram);

    Task<Socket> createClientSocketBT(const DeviceData& data);
}
