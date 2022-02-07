// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A system to perform I/O asynchronously using a completion-based model

#pragma once

#ifdef _WIN32
#include <WinSock2.h>
#endif

#include "sys/error.hpp"

namespace Async {
    struct AsyncData {
#ifdef _WIN32
        // This is used for overlapped I/O.
        // This must be the first field in this struct, as type punning is used to pass an entire instance to an IOCP
        // callback through a pointer to this field.
        OVERLAPPED overlapped{};
#endif

        // If the socket is connected to a remote host
        bool connected = false;
    };

    int init();

    void cleanup();

    int add(SOCKET sockfd);

#ifdef _WIN32
    HANDLE getCompletionPort();
#endif
}
