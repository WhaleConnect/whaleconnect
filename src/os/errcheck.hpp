// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <concepts>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>

#include "error.hpp"

#define FN(f, ...) SysFn(#f, f __VA_OPT__(, ) __VA_ARGS__)

// Predicate functions to check success based on return code
constexpr auto checkTrue = [](auto rc) { return static_cast<bool>(rc); };  // Check if return code evaluates to true
constexpr auto checkZero = [](auto rc) { return rc == NO_ERROR; };         // Check if return code equals zero
constexpr auto checkNonError = [](auto rc) { return rc != SOCKET_ERROR; }; // Check if return code is not -1

// Projection functions to turn return codes into numeric error codes
constexpr auto useLastError = [](auto) { return System::getLastError(); }; // Ignore return code, use global error value
constexpr auto useReturnCode = [](auto rc) { return rc; };                 // Use return code as-is
constexpr auto useReturnCodeNeg = [](auto rc) { return -rc; };             // Use negated return code

// Structure to contain a function with its textual name and arguments.
template <class Fn, class... Args>
requires std::invocable<Fn, Args...>
struct SysFn {
    using RType = std::invoke_result_t<Fn, Args...>; // Return type

    std::string name;          // Function name
    std::function<RType()> fn; // Function with arguments to call

    // Sets the function and its arguments.
    SysFn(std::string_view s, Fn p, const Args&... args) : name(s), fn([p, &args...]() { return p(args...); }) {}

    // Invokes the function.
    RType operator()() const { return fn(); }
};

// Calls a system function, and throws an exception if its return code does not match a success value.
// The success condition and thrown error code can be changed with predicate and projection functions.
// TODO: Remove typename in Clang 16
template <class Fn, class... Args, class Pred = decltype(checkNonError), class Proj = decltype(useLastError)>
typename SysFn<Fn, Args...>::RType call(const SysFn<Fn, Args...>& fn, Pred checkFn = {}, Proj transformFn = {},
                                        System::ErrorType type = System::ErrorType::System) {
    auto rc = fn();
    System::ErrorCode error = transformFn(rc);

    return (checkFn(rc) || !System::isFatal(error)) ? rc : throw System::SystemError{ error, type, fn.name };
}
