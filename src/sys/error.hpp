// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief Facilities for handling system errors
*/

#pragma once

#include <string>
#include <string_view>
#include <stdexcept>

#ifdef _WIN32
#include <WinSock2.h>
#endif

// Error status codes
#ifndef _WIN32
constexpr auto INVALID_SOCKET = -1; /**< An invalid socket descriptor */
constexpr auto SOCKET_ERROR = -1; /**< An error has occurred (returned from a function) */
constexpr auto NO_ERROR = 0; /**< Done successfully (returned from a function) */
#endif

namespace System {
    /**
     * @brief A generalized type for system error codes across platforms.
    */
    using ErrorCode =
#ifdef _WIN32
        DWORD
#else
        int
#endif
        ;

    /**
     * @brief Gets the last error code. This is platform-specific.
     * @return The error code
    */
    ErrorCode getLastError();

    /**
     * @brief Sets the last error code. This is platform-specific.
     * @param code The new error code
    */
    void setLastError(ErrorCode code);

    /**
     * @brief Checks if an error code should be handled as a fatal error.
     * @param code The error code to check
     * @return If the error code should be handled specially
    */
    bool isFatal(ErrorCode code);

    /**
     * @brief Where an error came from.
    */
    enum class ErrorType {
        System, /**< From socket functions or other OS APIs */
        AddrInfo /**< From a call to @p getaddrinfo() */
    };

    /**
     * @brief An exception structure containing details of an error.
    */
    struct SystemError : public std::exception {
        ErrorCode code = NO_ERROR; /**< The platform-specific error code */
        ErrorType type = ErrorType::System; /**< The type of the error */
        std::string fnName; /**< The name of the function that caused the error */

        /**
         * @brief Constructs an object representing success.
        */
        SystemError() = default;

        /**
         * @brief Constructs an object representing a specific error.
         * @param code The error code
         * @param type The error type
         * @param fnName The name of the function the error is from
        */
        SystemError(ErrorCode code, ErrorType type, std::string_view fnName) : code(code), type(type), fnName(fnName) {}

        /**
         * @brief Checks if this object represents a fatal error.
         * @sa System::isFatal()
        */
        explicit operator bool() const { return isFatal(code); }

        /**
         * @brief Represents this object as a readable string.
         * @return A string containing error information
        */
        std::string formatted() const;
    };
}
