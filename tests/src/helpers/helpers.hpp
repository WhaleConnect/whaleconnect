// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <coroutine>
#include <exception>
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
void runSync(const Awaitable auto& fn, bool useRunLoop = false) {
    bool hasRunLoop = OS_MACOS && useRunLoop;

    // Completion tracking
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
            completed = true;
            completed.notify_one();
        }
    }();

    // Wait for the completion condition
    if (hasRunLoop) {
#if OS_MACOS
        CFRunLoopRun();
#endif
    } else {
        completed.wait(false);
    }

    // Rethrow any exceptions
    if (ptr) std::rethrow_exception(ptr);
}

// Loads test settings from the JSON file.
nlohmann::json loadSettings();
