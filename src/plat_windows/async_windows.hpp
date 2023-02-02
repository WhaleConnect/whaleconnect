// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_WINDOWS
#include <WinSock2.h>

#include "os/async.hpp"

namespace Async {
    // Adds a socket to the I/O queue.
    void add(SOCKET sockfd);
}
#endif
