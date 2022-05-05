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
 * The thrown exception can be modified with @p type and @p errorValue.<br>
 * Since this macro is defined as a lambda, the function's return code can also be accessed.
*/
#define CALL_SYSTEM_FUNCTION_BASE(f, type, successCheck, errorValue, ...) [&] { \
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
#define CALL_SYSTEM_FUNCTION_TYPE(f, type, successCheck, ...) \
CALL_SYSTEM_FUNCTION_BASE(f, type, successCheck, System::getLastError(), __VA_ARGS__)

/**
 * @brief Throws if a function fails.
*/
#define CALL_SYSTEM_FUNCTION(f, successCheck, ...) \
CALL_WITH_DEFAULT_TYPE(CALL_SYSTEM_FUNCTION_TYPE, f, successCheck, __VA_ARGS__)

/**
 * @brief Throws if the return code is not true. The exception's error type can be set.
*/
#define CALL_EXPECT_TRUE_TYPE(f, type, ...) CALL_SYSTEM_FUNCTION_TYPE(f, type, rc, __VA_ARGS__)

/**
 * @brief Throws if the return code is not true.
*/
#define CALL_EXPECT_TRUE(f, ...) CALL_WITH_DEFAULT_TYPE(CALL_EXPECT_TRUE_TYPE, f, __VA_ARGS__)

/**
 * @brief Throws if the return code is not zero. The exception's error type can be set.
*/
#define CALL_EXPECT_ZERO_TYPE(f, type, ...) CALL_SYSTEM_FUNCTION_TYPE(f, type, rc == NO_ERROR, __VA_ARGS__)

/**
 * @brief Throws if the return code is not zero.
*/
#define CALL_EXPECT_ZERO(f, ...) CALL_WITH_DEFAULT_TYPE(CALL_EXPECT_ZERO_TYPE, f, __VA_ARGS__)

/**
 * @brief Throws if the return code is equal to @p SOCKET_ERROR. The exception's error type can be set.
*/
#define CALL_EXPECT_NONERROR_TYPE(f, type, ...) CALL_SYSTEM_FUNCTION_TYPE(f, type, rc != SOCKET_ERROR, __VA_ARGS__)

/**
 * @brief Throws if the return code is equal to @p SOCKET_ERROR.
*/
#define CALL_EXPECT_NONERROR(f, ...) CALL_WITH_DEFAULT_TYPE(CALL_EXPECT_NONERROR_TYPE, f, __VA_ARGS__)

/**
 * @brief Throws if the return code is not true. The exception's error type can be set.
 *
 * The exception's error value will be set to the function's return value.
*/
#define CALL_EXPECT_ZERO_RC_ERROR_TYPE(f, type, ...) CALL_SYSTEM_FUNCTION_BASE(f, type, rc == NO_ERROR, rc, __VA_ARGS__)

/**
 * @brief Throws if the return code is not true.
 *
 * The exception's error value will be set to the function's return value.
*/
#define CALL_EXPECT_ZERO_RC_ERROR(f, ...) CALL_WITH_DEFAULT_TYPE(CALL_EXPECT_ZERO_RC_ERROR_TYPE, f, __VA_ARGS__)
