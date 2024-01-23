// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <WinSock2.h>

module os.async.platform;
import os.async.platform.internal;
import os.errcheck;

void Async::add(SOCKET sockfd) {
    check(CreateIoCompletionPort(reinterpret_cast<HANDLE>(sockfd), Internal::completionPort, 0, 0), checkTrue);
}
