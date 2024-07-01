// Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "async.hpp"

#include <atomic>
#include <coroutine>
#include <forward_list>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "utils/task.hpp"

class WorkerThread {
    unsigned int queueEntries;

    std::vector<std::coroutine_handle<>> workQueue;
    std::mutex queueMutex;
    std::atomic_size_t numWork = 0;
    std::atomic_bool hasWork = false;
    std::atomic_bool shouldStop = false;

    std::unique_ptr<Async::EventLoop> eventLoop;
    std::thread thread;
    std::thread::id id;

    void loop();

    void stop() {
        shouldStop.store(true, std::memory_order_relaxed);
        hasWork.store(true, std::memory_order_relaxed);
        hasWork.notify_one();
    }

public:
    WorkerThread(unsigned int queueEntries) :
        queueEntries(queueEntries), thread(&WorkerThread::loop, this), id(thread.get_id()) {}

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
    // Initialize event loop on this thread (needed for single issuer optimization on Linux)
    // numThreads in an event loop constructor is only used on Windows, and only with the first instantiation.
    // Since the main event loop is initialized first, 0 is passed here to avoid storing another value in this class.
    eventLoop = std::make_unique<Async::EventLoop>(0, queueEntries);

    while (true) {
        bool expected = true;
        if (!hasWork.compare_exchange_weak(expected, false, std::memory_order_relaxed)) {
            if (eventLoop->size() == 0) {
                // Make thread idle to save CPU cycles
                hasWork.wait(false, std::memory_order_relaxed);
            } else {
                // There are outstanding I/O events, check back periodically
                using namespace std::literals;
                std::this_thread::sleep_for(200ms);
            }
        }

        if (shouldStop.load(std::memory_order_relaxed)) break;

        eventLoop->runOnce();

        // Swap the work queue with an empty queue. This performs the following actions:
        //   - Clears the work queue
        //   - Makes the data isolated from other threads so the mutex can be locked for minimal time
        //   - Consumes less memory compared to a full copy

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

Task<> queueFnToThread(WorkerThread& thread, std::function<Task<bool>()> f) {
    Async::CompletionResult result;
    co_await result;

    // Copy the given function to preserve it when the coroutine is resumed
    auto tmp = f;

    thread.push(result.coroHandle);
    co_await std::suspend_always{};

    if (co_await tmp()) co_await queueFnToThread(thread, tmp);
}

unsigned int Async::init(unsigned int numThreads, unsigned int queueEntries) {
    // If 0 threads are specified, the number is chosen with hardware_concurrency.
    // If the number of supported threads cannot be determined, no worker threads are created.
    // The number of threads created is (desired number) - 1 since the main thread also runs an event loop.
    unsigned int realNumThreads = numThreads == 0 ? std::max(std::thread::hardware_concurrency(), 1U) : numThreads;
    eventLoop.emplace(realNumThreads, queueEntries);

    if (realNumThreads > 1)
        for (unsigned int i = 0; i < realNumThreads - 1; i++) threads.emplace_front(queueEntries);

    return realNumThreads;
}

void Async::cleanup() {
    threads.clear();
}

void Async::submit(const Operation& op) {
    std::thread::id currentThread = std::this_thread::get_id();

    // Push I/O to the worker thread that corresponds to the thread this function is running on
    // A coroutine will never leave a thread and will resume on the thread it suspended on.
    for (auto i = threads.begin(); i != threads.end(); i++) {
        if (i->getID() == currentThread) {
            i->pushIO(op);
            return;
        }
    }

    // If there is no corresponding worker thread, the operation was submitted in the main event loop
    eventLoop->push(op);
}

Task<> Async::queueToThread() {
    CompletionResult result;
    co_await result;

    WorkerThreadPool::iterator least;
    auto leastCount = std::numeric_limits<std::size_t>::max();

    for (auto i = threads.begin(); i != threads.end(); i++) {
        std::size_t size = i->size();

        // Immediately add work to any thread that is idle and waiting for work
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

    // If no threads are idle, push to the one with the least amount of work for even work distribution
    least->push(result.coroHandle);
    co_await std::suspend_always{};
}

void Async::queueToThreadEx(std::thread::id id, std::function<Task<bool>()> f) {
    bool allThreads = id == std::thread::id{};

    for (auto i = threads.begin(); i != threads.end(); i++)
        if (allThreads || i->getID() == id) queueFnToThread(*i, f);
}

void Async::handleEvents(bool wait) {
    eventLoop->runOnce(wait);
}
