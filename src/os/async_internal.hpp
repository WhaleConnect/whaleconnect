// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <coroutine>
#include <exception>

#include "async.hpp"

namespace Async::Internal {
    // Constant to indentify an interrupt operation to stop the worker threads.
    constexpr auto ASYNC_INTERRUPT = 1;

    // Initializes the background thread pool.
    void init(unsigned int numThreads, unsigned int queueEntries);

    // Signals all threads to exit.
    void stopThreads(unsigned int numThreads);

    // Cleans up system resources.
    void cleanup();

    // Handles asynchronous operations.
    void worker(unsigned int threadNum);
}
