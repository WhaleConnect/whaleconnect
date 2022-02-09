// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
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

    /// <summary>
    /// Get the last error code.
    /// </summary>
    ErrorCode getLastErr();

    /// <summary>
    /// Set the last error code.
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

    template <class T = void>
    class MayFail {
        static constexpr bool _isVoid = std::is_void_v<T>;

        using Type = std::conditional_t<_isVoid, bool, T>;
        using Optional = std::conditional_t<_isVoid, Type, std::optional<Type>>;

        ErrorCode _errCode;
        Optional _optVal;

    public:
        MayFail(Type value) : _optVal(value), _errCode(getLastErr()) {}

        ErrorCode error() { return _errCode; }

        Type value() {
            if constexpr (_isVoid) return _optVal;
            else return _optVal.value();
        }
    };
}
