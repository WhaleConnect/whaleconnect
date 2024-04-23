// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <utility>
#include <source_location>

#include "os/error.hpp"

// clang-format off
// Predicate functors to check success based on return code
inline struct CheckTrue { bool operator()(auto rc) { return static_cast<bool>(rc); } } checkTrue;
inline struct CheckZero { bool operator()(auto rc) { return std::cmp_equal(rc, 0); } } checkZero;
inline struct CheckNonError { bool operator()(auto rc) { return std::cmp_not_equal(rc, -1); } } checkNonError;

// Projection functors to turn return codes into numeric error codes
inline struct UseLastError { System::ErrorCode operator()(auto) { return System::getLastError(); } } useLastError;
inline struct UseReturnCode { System::ErrorCode operator()(auto rc) { return rc; } } useReturnCode;
inline struct UseReturnCodeNeg { System::ErrorCode operator()(auto rc) { return -rc; } } useReturnCodeNeg;

// clang-format on

// Calls a system function, and throws an exception if its return code does not match a success value.
// The success condition and thrown error code can be changed with predicate and projection functions.
template <class T, class Pred = CheckNonError, class Proj = UseLastError>
inline T check(const T& rc, Pred checkFn = {}, Proj transformFn = {}, System::ErrorType type = System::ErrorType::System,
    const std::source_location& location = std::source_location::current()) {
    System::ErrorCode code = transformFn(rc);
    return checkFn(rc) || !System::isFatal(code) ? rc : throw System::SystemError{ code, type, location };
}
