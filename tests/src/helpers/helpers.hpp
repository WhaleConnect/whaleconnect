// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <exception>
#include <mutex>
#include <type_traits>

#if OS_MACOS
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <nlohmann/json.hpp>

#include "os/error.hpp"
#include "utils/task.hpp"

// Concept to ensure a function type is an awaitable coroutine.
template <class T>
concept Awaitable = requires (T) { typename std::coroutine_traits<std::invoke_result_t<T>>::promise_type; };

// Runs a coroutine synchronously.
// Bluetooth functions on macOS require a run loop for events.
template <class Fn>
requires Awaitable<Fn>
void runSync(const Fn& fn, [[maybe_unused]] bool useRunLoop = false) {
#if OS_MACOS
    bool hasRunLoop = useRunLoop;
#else
    bool hasRunLoop = false;
#endif

    // Completion tracking
    std::mutex m;
    std::condition_variable cond;
    std::atomic_bool completed = false;

    // Keep track of exceptions thrown in the coroutine
    // Exceptions won't propagate out of the coroutine so they must be rethrown manually for the tests to catch them.
    std::exception_ptr ptr;

    // Run an outer coroutine, but don't await it
    // TODO: Remove empty parentheses in C++23
    [&]() -> Task<> {
        try {
            // Await the given coroutine
            co_await fn();
        } catch (const System::SystemError&) {
            // Store thrown exceptions
            ptr = std::current_exception();
        }

        // Either the coroutine finished, or it threw an exception
        if (hasRunLoop) {
#if OS_MACOS
            CFRunLoopStop(CFRunLoopGetCurrent());
#endif
        } else {
            std::unique_lock lock{ m };
            completed = true;
            lock.unlock();
            cond.notify_one();
        }
    }();

    // Wait for the completion condition
    if (hasRunLoop) {
#if OS_MACOS
        CFRunLoopRun();
#endif
    } else {
        std::unique_lock lock{ m };
        cond.wait(lock, [&completed] {
            return completed.load();
        });
    }

    // Rethrow any exceptions
    if (ptr) std::rethrow_exception(ptr);
}

// Loads test settings from the JSON file.
nlohmann::json loadSettings();
