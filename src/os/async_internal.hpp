// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm> // std::max()
#include <coroutine>
#include <thread>

#include "async.hpp"

namespace Async::Internal {
    // Constant to indentify an interrupt operation to stop the worker threads.
    constexpr auto ASYNC_INTERRUPT = 1;

    // Structure to contain the result of calling the worker function once.
    struct WorkerResult {
        bool interrupted = false;           // If the thread was interrupted while waiting
        std::coroutine_handle<> coroHandle; // The coroutine handle to resume
    };

    // Casts a pointer type to a completion result structure.
    template <class T>
    CompletionResult& toResult(T* ptr) {
        return *static_cast<CompletionResult*>(ptr);
    }

    inline WorkerResult resultInterrupted() { return { true, nullptr }; }

    inline WorkerResult resultError() { return { false, nullptr }; }

    inline WorkerResult resultSuccess(CompletionResult& result) { return { false, result.coroHandle }; }

    void init();

    void stopThreads();

    void cleanup();

    WorkerResult worker();
}
