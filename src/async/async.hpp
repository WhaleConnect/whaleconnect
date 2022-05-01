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
    struct CompletionResult
#ifdef _WIN32
        // Inherit from OVERLAPPED on Windows to pass this struct as IOCP user data
        // https://devblogs.microsoft.com/oldnewthing/20101217-00/?p=11983
        : OVERLAPPED
#endif
    {
        std::coroutine_handle<> coroHandle;
        System::ErrorCode error;
        int numBytes;

        System::ErrorCode errorResult() { return error; }

        bool await_ready() const noexcept { return static_cast<bool>(coroHandle); }

        bool await_suspend(std::coroutine_handle<> coroutine) noexcept {
            coroHandle = coroutine;
            return false;
        }

        void await_resume() const noexcept {}
    };

    void init();

    void cleanup();

    void add(SOCKET sockfd);
}
