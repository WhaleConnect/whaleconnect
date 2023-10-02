// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_WINDOWS
#include <bit>

#include <WinSock2.h>

#include "os/check.hpp"

module os.async.platform;
import os.async.platform.internal;
import os.errcheck;

void Async::add(SOCKET sockfd) {
    CHECK(CreateIoCompletionPort(std::bit_cast<HANDLE>(sockfd), Internal::completionPort, 0, 0), checkTrue);
}
#endif
