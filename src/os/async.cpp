// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module os.async;
import external.std;
import os.async.internal;

std::queue<std::coroutine_handle<>> queuedHandles;
std::mutex queueMutex;

Async::Instance::Instance(unsigned int numThreads, unsigned int queueEntries) :
    workerThreadPool(numThreads == 0 ? std::max(std::thread::hardware_concurrency(), 1U) : numThreads) {
    // If 0 threads are specified, the number is chosen with hardware_concurrency.
    // If the number of supported threads cannot be determined, 1 is created.
    Internal::init(static_cast<unsigned int>(workerThreadPool.size()), queueEntries);

    // Populate thread pool
    for (unsigned int i = 0; i < workerThreadPool.size(); i++)
        workerThreadPool[i] = std::thread{ Internal::worker, i };
}

Async::Instance::~Instance() {
    Internal::stopThreads(static_cast<unsigned int>(workerThreadPool.size()));

    // Join threads
    // Manual join calls are used instead of a jthread so the program can wait (briefly) for all threads to exit
    // before cleaning up.
    for (auto& i : workerThreadPool)
        if (i.joinable()) i.join();

    Internal::cleanup();
}

void Async::handleEvents() {
    std::scoped_lock lock{ queueMutex };
    while (!queuedHandles.empty()) {
        auto handle = queuedHandles.front();
        queuedHandles.pop();
        handle();
    }
}

void Async::queueForCompletion(const Async::CompletionResult& result) {
    std::scoped_lock lock{ queueMutex };
    queuedHandles.push(result.coroHandle);
}
