// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A system to perform I/O asynchronously using a completion-based model

#pragma once

#ifdef _WIN32
#include <WinSock2.h>
#endif

#include <coroutine>

#include "sys/error.hpp"

namespace Async {
    struct CompletionResult {
#ifdef _WIN32
        // This is used for overlapped I/O.
        // This must be the first field in this struct, as type punning is used to pass an entire instance to an IOCP
        // callback through a pointer to this field.
        OVERLAPPED overlapped{};
#endif

        int numBytes = 0;
        int error = 0;
        std::coroutine_handle<> coroHandle;

        bool await_ready() const noexcept { return false; }

        void await_suspend(std::coroutine_handle<> coroutine) noexcept {
            coroHandle = coroutine;
            coroHandle();
        }

        void await_resume() const noexcept {}
    };

    System::MayFail<> init();

    void cleanup();

    System::MayFail<> add(SOCKET sockfd);
}
