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

Async::Instance::Instance(unsigned int numThreads) :
    _workerThreadPool((numThreads == 0) ? std::max(std::thread::hardware_concurrency(), 1U) : numThreads) {
    // If 0 threads are specified, the number is chosen with hardware_concurrency.
    // If the number of supported threads cannot be determined, 1 is created.
    Internal::init(static_cast<unsigned int>(_workerThreadPool.size()));

    // Populate thread pool
    // TODO: Use views::enumerate() in C++23
    for (size_t i = 0; i < _workerThreadPool.size(); i++)
        _workerThreadPool[i] = std::thread{ Async::Internal::worker, i };
}

Async::Instance::~Instance() {
    Internal::stopThreads(static_cast<unsigned int>(_workerThreadPool.size()));

    // Join threads
    // Manual join calls are used instead of a jthread so the program can wait (briefly) for all threads to exit
    // before cleaning up.
    for (auto& i : _workerThreadPool)
        if (i.joinable()) i.join();

    Internal::cleanup();
}
