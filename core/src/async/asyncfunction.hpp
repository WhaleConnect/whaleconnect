// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A wrapper class for std::async()

#pragma once

#include <future> // std::future, std::async()
#include <chrono> // std::chrono
#include <system_error> // std::system_error

/// <summary>
/// A wrapper class for `std::async()`.
/// </summary>
/// <typeparam name="T">The type of the value returned asynchronously</typeparam>
template <class T>
class AsyncFunction {
    T _value{}; // The value that will be returned
    std::future<T> _fut; // The future object
    bool _firstRun = false; // If a successful run() call occurred at least once
    bool _error = false; // If an error occurred during calling run()
    bool _done = false; // If the function has completed (NOT if it can return a value)

public:
    /// <summary>
    /// Run a function asynchronously.
    /// </summary>
    /// <typeparam name="Fn">The function to run</typeparam>
    /// <typeparam name="...Args">Additional arguments to pass to the function</typeparam>
    /// <param name="fn">The function to run</param>
    /// <param name="...args">Additional arguments to pass to the function</param>
    template <class Fn, class... Args>
    void run(Fn&& fn, Args&&... args) {
        try {
            _fut = std::async(std::launch::async, fn, std::forward<Args>(args)...);
            _firstRun = true;
            _error = _done = false;
        } catch (const std::system_error&) {
            // Something happened (thread failed to start)
            _error = _done = true;
        }
    }

    /// <summary>
    /// Check if the class has run a function successfully at least once (first run).
    /// </summary>
    /// <returns>If a first run occurred</returns>
    bool firstRun() { return _firstRun; }

    /// <summary>
    /// Check if the run() call failed.
    /// </summary>
    /// <returns>If the function failed to run</returns>
    bool error() { return _error; }

    /// <summary>
    /// Check the ready state.
    /// </summary>
    /// <returns>If the function can return a value</returns>
    /// <remarks>
    /// This function's state is invalidated by a call to value() - if this function returns true, then value()
    /// is called, the next call to this function will return false (get() can only be called once on a future object).
    /// To check if the function has finished executing, use checkDone().
    /// </remarks>
    bool ready() {
        // The && should short-circuit here - wait_for() should not execute if valid() returns false.
        // This is the desired and expected behavior, since waiting on an invalid future throws an exception.
        using namespace std::literals;
        return _fut.valid() && (_fut.wait_for(0s) == std::future_status::ready);
    }

    /// <summary>
    /// Check the finished state.
    /// </summary>
    /// <returns>If the function has finished executing</returns>
    bool checkDone() {
        if (ready()) _done = true;
        return _done;
    }

    /// <summary>
    /// Get the value returned from the function.
    /// </summary>
    /// <returns>The function return value</returns>
    /// <remarks>
    /// This function caches the result of `std::future::get()`. A value can still be obtained even if the internal
    /// future is no longer ready, given the successful retreival of a prior run, which will be returned.
    /// </remarks>
    T value() {
        if (ready()) _value = _fut.get();
        return _value;
    }
};
