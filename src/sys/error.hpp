// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Functions for getting and setting the last error code

#pragma once

#include <string>
#include <stdexcept>

#ifndef _WIN32
// Error status codes
constexpr auto INVALID_SOCKET = -1; // An invalid socket descriptor
constexpr auto SOCKET_ERROR = -1; // An error has occurred (returned from a function)
constexpr auto NO_ERROR = 0; // Done successfully (returned from a function)
#endif

namespace System {
#ifdef _WIN32
    using ErrorCode = DWORD;
#else
    using ErrorCode = int;
#endif

    /// <summary>
    /// Get the last error code.
    /// </summary>
    ErrorCode getLastError();

    /// <summary>
    /// Set the last error code.
    /// </summary>
    void setLastError(ErrorCode code);

    bool isFatal(ErrorCode code);

    enum class ErrorType { System, AddrInfo };

    struct SystemError : public std::exception {
        ErrorCode code = NO_ERROR;
        ErrorType type = ErrorType::System;
        std::string fnName;

        SystemError() = default;

        SystemError(ErrorCode code, ErrorType type, std::string function) : code(code), type(type), fnName(function) {}

        operator bool() const { return isFatal(code); }

        std::string formatted() const;
    };
}
