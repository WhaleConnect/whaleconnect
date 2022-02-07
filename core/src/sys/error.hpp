// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Functions for getting and setting the last error code

#pragma once

#include <string>
#include <type_traits>
#include <optional>

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

    namespace Internal {
        class MayFailBaseVoid {
            bool _failed = false;

        public:
            explicit MayFailBaseVoid(bool failed) : _failed(failed) {}

            operator bool() { return _failed; };
        };

        template <class T>
        class MayFailBaseValue {
            std::optional<T> _value;

        public:
            explicit MayFailBaseValue(T value) : _value(ValueType) {}

            operator bool() { return !_value };
        };
    }

    /// <summary>
    /// Get the error code of the last socket error.
    /// </summary>
    ErrorCode getLastErr();

    /// <summary>
    /// Set the last socket error code.
    /// </summary>
    void setLastErr(ErrorCode err);

    /// <summary>
    /// Format an error code into a readable string.
    /// </summary>
    /// <param name="code">The error code to format</param>
    /// <returns>The formatted string with code and description</returns>
    std::string formatErr(ErrorCode code);

    /// <summary>
    /// Format the last error code into a readable string.
    /// </summary>
    /// <returns>The formatted string with code and description</returns>
    std::string formatLastErr();
}
