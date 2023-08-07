// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <coroutine>
#include <exception>

#include "async.hpp"

namespace Async::Internal {
    // Constant to indentify an interrupt operation to stop the worker threads.
    constexpr auto ASYNC_INTERRUPT = 1;

    // Exception representing an interrupted worker call.
    struct WorkerInterruptedError : std::exception {};

    // Exception representing insufficient data to satisfy an operation.
    struct WorkerNoDataError : std::exception {};

    // Casts a pointer type to a completion result structure.
    template <class T>
    CompletionResult& toResult(T* ptr) {
        if (!ptr) throw WorkerNoDataError{};
        return *static_cast<CompletionResult*>(ptr);
    }

    // Initializes the background thread pool.
    void init(unsigned int numThreads);

    // Signals all threads to exit.
    void stopThreads(unsigned int numThreads);

    // Cleans up system resources.
    void cleanup();

    // Handles asynchronous operations.
    void worker(int threadNum);
}
