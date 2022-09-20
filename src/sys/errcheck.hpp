// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief Macros for calling system functions and throwing exceptions if they fail
*/

#pragma once

#include "error.hpp"

/**
 * @brief Calls a system function, and throws an exception if its return code does not match a success value.
 *
 * The thrown exception can be modified with @p type and @p errorValue. Since this macro is defined as a lambda, the
 * function's return code can also be accessed.
*/
#define CALL_FN_BASE(f, type, successCheck, errorValue, ...) [&] { \
    auto rc = f(__VA_ARGS__); \
    return ((successCheck) || !System::isFatal(System::getLastError())) \
        ? rc \
        : throw System::SystemError{ static_cast<System::ErrorCode>(errorValue), type, #f }; \
}()

/**
 * @brief Calls one of the @p _TYPE macros with the type being @p System.
*/
#define CALL_WITH_DEFAULT_TYPE(macro, f, ...) macro(f, System::ErrorType::System, __VA_ARGS__)

/**
 * @brief Throws if a function fails. The exception's error type can be set.
*/
#define CALL_FN_TYPE(f, type, check, ...) CALL_FN_BASE(f, type, check, System::getLastError(), __VA_ARGS__)

/**
 * @brief See behavior for @p CALL_FN_TYPE.
*/
#define CALL_FN(f, check, ...) CALL_WITH_DEFAULT_TYPE(CALL_FN_TYPE, f, check, __VA_ARGS__)

/**
 * @brief Throws if the return code is not true. The exception's error type can be set.
*/
#define EXPECT_TRUE_TYPE(f, type, ...) CALL_FN_TYPE(f, type, rc, __VA_ARGS__)

/**
 * @brief See behavior for @p EXPECT_TRUE_TYPE.
*/
#define EXPECT_TRUE(f, ...) CALL_WITH_DEFAULT_TYPE(EXPECT_TRUE_TYPE, f, __VA_ARGS__)

/**
 * @brief Throws if the return code is not zero. The exception's error type can be set.
*/
#define EXPECT_ZERO_TYPE(f, type, ...) CALL_FN_TYPE(f, type, rc == NO_ERROR, __VA_ARGS__)

/**
 * @brief See behavior for @p EXPECT_ZERO_TYPE.
*/
#define EXPECT_ZERO(f, ...) CALL_WITH_DEFAULT_TYPE(EXPECT_ZERO_TYPE, f, __VA_ARGS__)

/**
 * @brief Throws if the return code is equal to @p SOCKET_ERROR. The exception's error type can be set.
*/
#define EXPECT_NONERROR_TYPE(f, type, ...) CALL_FN_TYPE(f, type, rc != SOCKET_ERROR, __VA_ARGS__)

/**
 * @brief See behavior for @p EXPECT_NONERROR_TYPE.
*/
#define EXPECT_NONERROR(f, ...) CALL_WITH_DEFAULT_TYPE(EXPECT_NONERROR_TYPE, f, __VA_ARGS__)

/**
 * @brief Throws if the return code is not true. The exception's error type can be set.
 *
 * The exception's error value will be set to the function's return value.
*/
#define EXPECT_ZERO_RC_TYPE(f, type, ...) CALL_FN_BASE(f, type, rc == NO_ERROR, rc, __VA_ARGS__)

/**
 * @brief See behavior for @p EXPECT_ZERO_RC_TYPE.
*/
#define EXPECT_ZERO_RC(f, ...) CALL_WITH_DEFAULT_TYPE(EXPECT_ZERO_RC_TYPE, f, __VA_ARGS__)

/**
 * @brief Throws if the return code is negative. The exception's error type can be set.
 *
 * The exception's error value will be set to the function's return value, negated. This macro can be used with newer
 * POSIX APIs that return >= 0 on success and @p -errno on failure.
*/
#define EXPECT_POSITIVE_RC_TYPE(f, type, ...) CALL_FN_BASE(f, type, rc >= 0, -rc, __VA_ARGS__)

/**
 * @brief See behavior for @p EXPECT_POSITIVE_RC_TYPE.
*/
#define EXPECT_POSITIVE_RC(f, ...) CALL_WITH_DEFAULT_TYPE(EXPECT_POSITIVE_RC_TYPE, f, __VA_ARGS__)
