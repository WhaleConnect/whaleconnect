// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "error.hpp"

// clang-format off

#define FN(f, ...) FnResult{ f(__VA_ARGS__), #f }

// Predicate functions to check success based on return code
constexpr auto checkTrue = [](auto rc) { return static_cast<bool>(rc); };  // Check if return code evaluates to true
constexpr auto checkZero = [](auto rc) { return rc == NO_ERROR; };         // Check if return code equals zero
constexpr auto checkNonError = [](auto rc) { return rc != SOCKET_ERROR; }; // Check if return code is not -1

// Projection functions to turn return codes into numeric error codes
constexpr auto useLastError = [](auto) { return System::getLastError(); }; // Ignore return code, use global error value
constexpr auto useReturnCode = [](auto rc) { return rc; };                 // Use return code as-is
constexpr auto useReturnCodeNeg = [](auto rc) { return -rc; };             // Use negated return code

// clang-format on

// Structure to contain a function's textual name and return code.
template <class T>
struct FnResult {
    T rc = 0;         // Return code
    std::string name; // Function name
};

#ifdef __clang__
// Clang does not yet support CTAD for aggregates (P1816R0)
template <class T>
FnResult(T, std::string) -> FnResult<T>;
#endif

// Calls a system function, and throws an exception if its return code does not match a success value.
// The success condition and thrown error code can be changed with predicate and projection functions.
template <class T, class Pred = decltype(checkNonError), class Proj = decltype(useLastError)>
T call(const FnResult<T>& fn, Pred checkFn = {}, Proj transformFn = {},
       System::ErrorType type = System::ErrorType::System) {
    T rc = fn.rc;
    System::ErrorCode error = transformFn(rc);

    return (checkFn(rc) || !System::isFatal(error)) ? rc : throw System::SystemError{ error, type, fn.name };
}
