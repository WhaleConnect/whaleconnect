// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "async.hpp"
#include "async_internal.hpp"

#include <algorithm> // std::max()
#include <system_error>
#include <thread>
#include <vector>

// Runs in each thread to handle completion results.
static void worker() {
    while (true) {
        auto result = Async::Internal::worker();

        if (result.interrupted) break;
        else if (result.coroHandle) result.coroHandle();
    }
}

Async::Instance::Instance(unsigned int numThreads) :
    _workerThreadPool((numThreads == 0) ? std::max(std::thread::hardware_concurrency(), 1U) : numThreads) {
    // If 0 threads are specified, the number is chosen with hardware_concurrency.
    // If the number of supported threads cannot be determined, 1 is created.
    Internal::init();

    // Populate thread pool
    for (auto& i : _workerThreadPool) i = std::thread{ worker };
}

Async::Instance::~Instance() {
    Internal::stopThreads();

    // Join threads
    // Manual join calls are used instead of a jthread so the program can wait (briefly) for all threads to exit
    // before cleaning up.
    for (auto& i : _workerThreadPool)
        if (i.joinable()) i.join();

    Internal::cleanup();
}
