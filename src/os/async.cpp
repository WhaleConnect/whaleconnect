// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "async.hpp"

#include <algorithm>
#include <forward_list>

using WorkerThreadPool = std::forward_list<Async::WorkerThread>;
WorkerThreadPool threads;

void Async::init(unsigned int numThreads, unsigned int queueEntries) {
    // If 0 threads are specified, the number is chosen with hardware_concurrency.
    // If the number of supported threads cannot be determined, 1 is created.
    unsigned int realNumThreads = numThreads == 0 ? std::max(std::thread::hardware_concurrency(), 1U) : numThreads;
    for (unsigned i = 0; i < realNumThreads; i++) threads.emplace_front(realNumThreads, queueEntries);
}

void Async::submit(const Operation& op) {
#if !OS_WINDOWS
    if (std::holds_alternative<Cancel>(op)) {
        for (auto& i : threads) i.push(op);
        return;
    }

    WorkerThreadPool::iterator least;
    auto leastCount = std::numeric_limits<std::size_t>::max();

    for (auto i = threads.begin(); i != threads.end(); i++) {
        std::size_t size = i->size();

        if (size == 0) {
            i->push(op);
            return;
        }

        if (size < leastCount) {
            leastCount = size;
            least = i;
        }
    }

    least->push(op);
#endif
}
