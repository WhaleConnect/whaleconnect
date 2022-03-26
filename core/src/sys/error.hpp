// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Functions for getting and setting the last error code

#pragma once

#include <string>
#include <type_traits>
#include <optional>
#include <utility> // std::forward()

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

    template <class T = void>
    class MayFail;

    /// <summary>
    /// Get the last error code.
    /// </summary>
    ErrorCode getLastErr();

    /// <summary>
    /// Set the last error code.
    /// </summary>
    void setLastErr(ErrorCode code);

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

template <class T>
class System::MayFail {
    static constexpr bool _isVoid = std::is_void_v<T>;

    using Type = std::conditional_t<_isVoid, bool, T>;
    using Optional = std::conditional_t<_isVoid, Type, std::optional<Type>>;

    ErrorCode _errCode = NO_ERROR;
    Optional _optVal{};

public:
    MayFail() = default;

    MayFail(std::nullopt_t) : _errCode(getLastErr()) {}

    template <class U>
    MayFail(U&& value) : _optVal(std::forward<U>(value)) {
        // If the value is invalid, store the last error code for retrieval
        if (!_optVal) _errCode = getLastErr();
    }

    ErrorCode error() const { return _errCode; }

    // TODO: Remove macro hack and duplicate accessors in C++23: https://stackoverflow.com/a/69486322

#define GET_VALUE if constexpr (_isVoid) return _optVal; \
            else return _optVal.value();

    Type& operator*() { GET_VALUE }

    const Type& operator*() const { GET_VALUE }

#undef GET_VALUE

    Type* operator->() { return &operator*(); }

    const Type* operator->() const { return &operator*(); }

    operator bool() const {
        // Check if the code is actually an error
        if (_errCode == NO_ERROR) return true;

#ifdef _WIN32
        // This error means an operation hasn't failed, it's still waiting.
        // Tell the calling function that there's no error, and it should check back later.
        if (_errCode == WSA_IO_PENDING) return true;
#endif

        // The error is fatal
        return false;
    }
};
