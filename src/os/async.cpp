// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "async.hpp"

#include <atomic>
#include <coroutine>
#include <forward_list>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "utils/task.hpp"

class WorkerThread {
    std::thread thread;
    std::thread::id id;
    std::atomic_bool shouldStop = false;
    std::unique_ptr<Async::EventLoop> eventLoop;
    unsigned int queueEntries;

    std::vector<std::coroutine_handle<>> workQueue;
    std::mutex queueMutex;
    std::atomic_size_t numWork = 0;
    std::atomic_bool hasWork = false;

    void loop();

    void stop() {
        shouldStop.store(true, std::memory_order_relaxed);
        hasWork.store(true, std::memory_order_relaxed);
        hasWork.notify_one();
    }

public:
    WorkerThread(unsigned int queueEntries) :
        thread(&WorkerThread::loop, this), id(thread.get_id()), queueEntries(queueEntries) {}

    ~WorkerThread() {
        stop();
        thread.join();
    }

    void push(std::coroutine_handle<> handle) {
        {
            std::scoped_lock lock{ queueMutex };
            workQueue.push_back(handle);
        }
        numWork.fetch_add(1, std::memory_order_relaxed);
        hasWork.store(true, std::memory_order_relaxed);
        hasWork.notify_one();
    }

    void pushIO(const Async::Operation& operation) {
        eventLoop->push(operation);
        hasWork.store(true, std::memory_order_relaxed);
        hasWork.notify_one();
    }

    std::size_t size() const {
        return numWork.load(std::memory_order_relaxed);
    }

    std::thread::id getID() const {
        return id;
    }
};

void WorkerThread::loop() {
    eventLoop = std::make_unique<Async::EventLoop>(queueEntries);

    while (true) {
        bool expected = true;
        if (!hasWork.compare_exchange_weak(expected, false, std::memory_order_relaxed))
            hasWork.wait(false, std::memory_order_relaxed);

        if (shouldStop.load(std::memory_order_relaxed)) break;

        eventLoop->runOnce();

        std::vector<std::coroutine_handle<>> tmp;
        {
            std::scoped_lock lock{ queueMutex };
            std::swap(tmp, workQueue);
        }

        for (const auto& i : tmp) {
            i();
            numWork.fetch_sub(1, std::memory_order_relaxed);
        }
    }
}

using WorkerThreadPool = std::forward_list<WorkerThread>;
WorkerThreadPool threads;
std::optional<Async::EventLoop> eventLoop;

void Async::init(unsigned int numThreads, unsigned int queueEntries) {
    // If 0 threads are specified, the number is chosen with hardware_concurrency.
    // If the number of supported threads cannot be determined, 1 is created.
    unsigned int realNumThreads = numThreads == 0 ? std::max(std::thread::hardware_concurrency(), 1U) : numThreads;
    for (unsigned int i = 0; i < realNumThreads; i++) threads.emplace_front(queueEntries);

    eventLoop.emplace(queueEntries);
}

void Async::submit(const Operation& op) {
    std::thread::id currentThread = std::this_thread::get_id();

    for (auto i = threads.begin(); i != threads.end(); i++) {
        if (i->getID() == currentThread) {
            i->pushIO(op);
            return;
        }
    }

    eventLoop->push(op);
}

Task<> Async::queueToThread() {
    CompletionResult result;
    co_await result;

    WorkerThreadPool::iterator least;
    auto leastCount = std::numeric_limits<std::size_t>::max();

    for (auto i = threads.begin(); i != threads.end(); i++) {
        std::size_t size = i->size();

        if (size == 0) {
            i->push(result.coroHandle);
            co_await std::suspend_always{};
            co_return;
        }

        if (size < leastCount) {
            leastCount = size;
            least = i;
        }
    }

    least->push(result.coroHandle);
    co_await std::suspend_always{};
}

void Async::handleEvents() {
    eventLoop->runOnce(false);
}
