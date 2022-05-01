// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Macros for calling system functions and throwing exceptions if they fail

#pragma once

#include "error.hpp"

#define CALL_SYSTEM_FUNCTION_BASE(f, type, successCheck, errorValue, ...) [&] { \
    auto rc = f(__VA_ARGS__); \
    return ((successCheck) || !System::isFatal(System::getLastError())) \
        ? rc \
        : throw System::SystemError{ static_cast<System::ErrorCode>(errorValue), type, #f }; \
}()

#define CALL_SYSTEM_FUNCTION_TYPE(f, type, successCheck, ...) \
CALL_SYSTEM_FUNCTION_BASE(f, type, successCheck, System::getLastError(), __VA_ARGS__)

#define CALL_SYSTEM_FUNCTION(f, successCheck, ...) \
CALL_SYSTEM_FUNCTION_TYPE(f, System::ErrorType::System, successCheck, __VA_ARGS__)

#define CALL_EXPECT_TRUE_TYPE(f, type, ...) CALL_SYSTEM_FUNCTION_TYPE(f, type, rc, __VA_ARGS__)

#define CALL_EXPECT_TRUE(f, ...) CALL_EXPECT_TRUE_TYPE(f, System::ErrorType::System, __VA_ARGS__)

#define CALL_EXPECT_ZERO_TYPE(f, type, ...) CALL_SYSTEM_FUNCTION_TYPE(f, type, rc == NO_ERROR,  __VA_ARGS__)

#define CALL_EXPECT_ZERO(f, ...) CALL_EXPECT_ZERO_TYPE(f, System::ErrorType::System, __VA_ARGS__)

#define CALL_EXPECT_POSITIVE_TYPE(f, type, ...) CALL_SYSTEM_FUNCTION_TYPE(f, type, rc != SOCKET_ERROR,  __VA_ARGS__)

#define CALL_EXPECT_POSITIVE(f, ...) CALL_EXPECT_POSITIVE_TYPE(f, System::ErrorType::System, __VA_ARGS__)

#define CALL_EXPECT_ZERO_RC_ERROR_TYPE(f, type, ...) CALL_SYSTEM_FUNCTION_BASE(f, type, rc == NO_ERROR, rc, __VA_ARGS__)

#define CALL_EXPECT_ZERO_RC_ERROR(f, ...) CALL_EXPECT_ZERO_RC_ERROR_TYPE(f, System::ErrorType::System, __VA_ARGS__)
