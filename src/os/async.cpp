// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "async.hpp"

#include "async_internal.hpp"

#include <algorithm> // std::max()
#include <exception>
#include <system_error>
#include <thread>
#include <vector>

#include "os/error.hpp"

// Runs in each thread to handle completion results.
static void worker() {
    while (true) {
        try {
            auto result = Async::Internal::worker();
            result.coroHandle();
        } catch (const Async::Internal::WorkerInterruptedError&) {
            break;    // Interrupted, break from loop
        } catch (const Async::Internal::WorkerNoDataError&) {
            continue; // No data to complete operation
        } catch (const System::SystemError&) {
            continue; // System call failed
        }
    }
}

Async::Instance::Instance(unsigned int numThreads) :
    _workerThreadPool((numThreads == 0) ? std::max(std::thread::hardware_concurrency(), 1U) : numThreads) {
    // If 0 threads are specified, the number is chosen with hardware_concurrency.
    // If the number of supported threads cannot be determined, 1 is created.
    Internal::init(_workerThreadPool.size());

    // Populate thread pool
    for (auto& i : _workerThreadPool) i = std::thread{ worker };
}

Async::Instance::~Instance() {
    Internal::stopThreads(_workerThreadPool.size());

    // Join threads
    // Manual join calls are used instead of a jthread so the program can wait (briefly) for all threads to exit
    // before cleaning up.
    for (auto& i : _workerThreadPool)
        if (i.joinable()) i.join();

    Internal::cleanup();
}
