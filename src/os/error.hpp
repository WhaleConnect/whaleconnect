// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#if OS_WINDOWS
#include <WinSock2.h>
#else
// Error status codes
constexpr auto SOCKET_ERROR = -1; // An error has occurred (returned from a function)
constexpr auto NO_ERROR = 0;      // Done successfully (returned from a function)
#endif

namespace System {
#if OS_WINDOWS
    using ErrorCode = DWORD;
#else
    using ErrorCode = int;
#endif

    // Gets the last error code. This is platform-specific.
    ErrorCode getLastError();

    // Checks if an error code should be handled as a fatal error.
    bool isFatal(ErrorCode code);

    // Where an error came from.
    enum class ErrorType {
        System,  // From socket functions or other OS APIs
        AddrInfo // From a call to getaddrinfo
    };

    // An exception structure containing details of an error.
    struct SystemError : std::exception {
        ErrorCode code = NO_ERROR;          // The platform-specific error code
        ErrorType type = ErrorType::System; // The type of the error
        std::string fnName;                 // The name of the function that caused the error

        // Constructs an object representing a specific error.
        SystemError(ErrorCode code, ErrorType type, std::string_view fnName) : code(code), type(type), fnName(fnName) {}

        // Checks if this object represents a fatal error.
        explicit operator bool() const { return isFatal(code); }

        // Represents this object as a readable string.
        [[nodiscard]] std::string formatted() const;
    };
}
