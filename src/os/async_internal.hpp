// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm> // std::max()
#include <coroutine>
#include <thread>

#include "async.hpp"

namespace Async::Internal {
    constexpr auto ASYNC_INTERRUPT = 1;

    // The number of threads to start
    // If the number of supported threads cannot be determined, 1 is created.
    inline const auto numThreads = std::max(std::thread::hardware_concurrency(), 1U);

    struct WorkerResult {
        bool interrupted = false;
        std::coroutine_handle<> coroHandle;
    };

    template <class T>
    CompletionResult& toResult(T* ptr) {
        return *static_cast<CompletionResult*>(ptr);
    }

    inline WorkerResult resultInterrupted() { return { true, nullptr }; }

    inline WorkerResult resultError() { return { false, nullptr }; }

    inline WorkerResult resultSuccess(CompletionResult& result) { return { false, result.coroHandle }; }

    bool invalid();

    void init();

    void stopThreads();

    void cleanup();

    WorkerResult worker();
}
