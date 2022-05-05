// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief A system to perform I/O asynchronously using a completion-based model
*/

#pragma once

#ifdef _WIN32
#include <WinSock2.h>
#endif

#include <coroutine>

#include "sys/error.hpp"

namespace Async {
    /**
     * @brief The information needed to resume a completion operation.
     *
     * This structure contains functions to make it an awaitable type. Calling @p co_await on an instance stores the
     * current coroutine handle to allow it to be resumed when the asynchronous operation finishes.
    */
    struct CompletionResult
#ifdef _WIN32
        // Inherit from OVERLAPPED on Windows to pass this struct as IOCP user data
        // https://devblogs.microsoft.com/oldnewthing/20101217-00/?p=11983
        : OVERLAPPED
#endif
    {
        std::coroutine_handle<> coroHandle; /**< The handle to the coroutine that started the operation */
        System::ErrorCode error; /**< The return code of the asynchronous function (returned to caller) */
        int numBytes; /**< The number of bytes transferred during the operation (returned to caller) */

        /**
         * @brief Gets the return code of the asynchronous function.
         * @return The return code
         *
         * This is provided for compatibility with the @p CALL_EXPECT_* macros in @p errcheck.hpp.
        */
        System::ErrorCode errorResult() { return error; }

        /**
         * @brief Checks if coroutine suspension is necessary.
         * @return If this object holds a coroutine handle
        */
        bool await_ready() const noexcept { return static_cast<bool>(coroHandle); }

        /**
         * @brief Stores the current coroutine handle to be resumed on completion.
         * @param coroutine The handle
         * @return False to resume the caller coroutine
        */
        bool await_suspend(std::coroutine_handle<> coroutine) noexcept {
            coroHandle = coroutine;
            return false;
        }

        /**
         * @brief Does nothing (nothing to do on coroutine resume).
        */
        void await_resume() const noexcept {}
    };

    /**
     * @brief Initializes the OS asynchronous I/O queue and starts the background thread pool.
    */
    void init();

    /**
     * @brief Cleans up the I/O queue and stops the thread pool.
    */
    void cleanup();

    /**
     * @brief Adds a socket to the I/O queue.
     * @param sockfd The socket file descriptor
    */
    void add(SOCKET sockfd);
}
