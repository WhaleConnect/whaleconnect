// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "async.hpp"
#include "async_internal.hpp"

#include <thread>
#include <vector>

// A vector of threads to serve as a thread pool
static std::vector<std::thread> workerThreadPool(Async::Internal::numThreads);

// Runs in each thread to handle completion results.
static void worker() {
    while (true) {
        auto result = Async::Internal::worker();

        if (result.interrupted) break;
        else if (result.coroHandle) result.coroHandle();
    }
}

void Async::init() {
    Internal::init();

    // Populate thread pool
    for (auto& i : workerThreadPool) i = std::thread{ worker };
}

void Async::cleanup() {
    // Signal threads to terminate
    Internal::stopThreads();

    // Join threads
    // Manual join calls are used instead of a jthread so the program can wait (briefly) for all threads to exit
    // before cleaning up.
    for (auto& i : workerThreadPool)
        if (i.joinable()) i.join();

    Internal::cleanup();
}
