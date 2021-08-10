// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string> // std::string
#include <future> // std::future, std::async()
#include <chrono> // std::chrono
#include <system_error> // std::system_error

#ifdef _WIN32
// Convert an integer type to a wstring.
// std::to_(w)string() has too many overloads to make a proper function wrapper
// So use a macro to do so instead
#define I_TO_WIDE std::to_wstring

typedef std::wstring widestr;

/// <summary>
/// Convert a UTF-8 string into a UTF-16 string.
/// </summary>
/// <param name="from">The input string</param>
/// <returns>The converted output string</returns>
std::wstring toWide(const std::string& from);

/// <summary>
/// Convert a UTF-16 string into a UTF-8 string.
/// </summary>
/// <param name="from">The input string</param>
/// <returns>The converted output string</returns>
std::string fromWide(const std::wstring& from);
#else
// No wide strings on other platforms
#define I_TO_WIDE std::to_string
#define toWide(from) from
#define fromWide(from) from

typedef std::string widestr;
#endif

/// <summary>
/// A wrapper class for std::async().
/// </summary>
/// <typeparam name="T">The type of the value returned asynchronously</typeparam>
/// <typeparam name="U">The type of the user data variable</typeparam>
template <class T, class U>
class AsyncFunction {
    T _value{}; // The value that will be returned
    U _userData{}; // Extra variable for storing anything
    std::future<T> _fut; // The future object
    bool _firstRun = false; // If a successful run() call occurred at least once
    bool _error = false; // If an error occurred during calling run()

public:
    /// <summary>
    /// Run a function asynchronously.
    /// </summary>
    /// <typeparam name="Fn">The function to run</typeparam>
    /// <typeparam name="...Args">Additional arguments to pass to the function</typeparam>
    /// <param name="fn">The function to run</param>
    /// <param name="...args">Additional arguments to pass to the function</param>
    template <class Fn, class... Args>
    void run(Fn&& fn, Args... args) {
        try {
            _firstRun = true;
            _fut = std::async(std::launch::async, fn, args...);
            _error = false;
        } catch (const std::system_error&) {
            // Something happened (thread failed to start)
            _error = true;
        }
    }

    /// <summary>
    /// Check if the class has run a function successfully at least once (first run).
    /// </summary>
    /// <returns>If a first run occurred</returns>
    bool firstRun() {
        return _firstRun;
    }

    /// <summary>
    /// Check the ready state.
    /// </summary>
    /// <returns>If the function has finished executing</returns>
    bool ready() {
        // The && should short-circuit here - wait_for() should not execute if valid() returns false.
        // This is the desired and expected behavior, since waiting on an invalid future throws an exception.
        return _fut.valid() && (_fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
    }

    /// <summary>
    /// Check if the run() call failed.
    /// </summary>
    /// <returns>If run() failed</returns>
    bool error() {
        return _error;
    }

    /// <summary>
    /// Get the value returned from the function.
    /// </summary>
    /// <returns></returns>
    T getValue() {
        if (ready()) _value = _fut.get();
        return _value;
    }

    /// <summary>
    /// Get the user data variable.
    /// </summary>
    /// <returns>The user data (modifiable)</returns>
    U& userData() {
        return _userData;
    }
};
