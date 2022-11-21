// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <coroutine>

#if OS_WINDOWS
#include <WinSock2.h>
#elif OS_LINUX
#include <liburing.h>
#endif

#include "error.hpp"
#include "utils/task.hpp"

namespace Async {
    // The information needed to resume a completion operation.
    //
    // This structure contains functions to make it an awaitable type. Calling co_await on an instance stores the
    // current coroutine handle to allow it to be resumed when the asynchronous operation finishes.
    struct CompletionResult
#if OS_WINDOWS
        // Inherit from OVERLAPPED on Windows to pass this struct as IOCP user data
        // https://devblogs.microsoft.com/oldnewthing/20101217-00/?p=11983
        : OVERLAPPED
#endif
    {
        std::coroutine_handle<> coroHandle; // The handle to the coroutine that started the operation
        System::ErrorCode error = NO_ERROR; // The return code of the asynchronous function (returned to caller)
        int res = 0; // The result the operation (returned to caller, exact meaning depends on operation)

        // Throws an exception if a fatal error occurred asynchronously.
        void checkError() const {
            if (System::isFatal(error))
                throw System::SystemError{ error, System::ErrorType::System, "<asynchronous function>" };
        }

        // Checks if coroutine suspension is necessary.
        [[nodiscard]] bool await_ready() const noexcept { return static_cast<bool>(coroHandle); }

        // Stores the current coroutine handle to be resumed on completion.
        bool await_suspend(std::coroutine_handle<> coroutine) noexcept {
            coroHandle = coroutine;
            return false;
        }

        // Does nothing (nothing to do on coroutine resume).
        void await_resume() const noexcept {}
    };

    // Awaits an asynchronous operation and returns the result of the operation's end function (if given).
    template <class Fn>
    Task<CompletionResult> run(Fn fn) {
        CompletionResult result{};
        co_await result;

        fn(result);

        co_await std::suspend_always{};
        result.checkError();

        co_return result;
    }

    // Initializes the OS asynchronous I/O queue and starts the background thread pool.
    void init();

    // Cleans up the I/O queue and stops the thread pool.
    void cleanup();

#if OS_WINDOWS
    // Adds a socket to the I/O queue.
    void add(SOCKET sockfd);
#elif OS_APPLE
    // Submits an event to the kernel queue.
    void submitKqueue(int ident, int filter, CompletionResult& result);
#elif OS_LINUX
    // Gets a submission queue entry from io_uring.
    io_uring_sqe* getUringSQE();

    // Submits entries into the io_uring submission queue.
    void submitRing();
#endif
}
