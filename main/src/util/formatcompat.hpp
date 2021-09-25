// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Compatibility layer for C++20 std::format()

// TODO: Remove this fallback file and just use `#include <format>` when all major compilers implement it

#pragma once

#if __has_include(<format>)
// #include <format> exists, directly use it
#include <format> // std::format() [C++20]
#else
// #include <format> doesn't exist, use {fmt} as a fallback
#include <string>
#include <string_view>

#include <fmt/core.h>

namespace std {
    /// <summary>
    /// Polyfill function for `std::format` [C++20].
    /// </summary>
    /// <typeparam name="...Args">Variadic arguments to format</typeparam>
    /// <param name="fString">The format string with formatting templates</param>
    /// <param name="...args">Arguments to format into the string</param>
    /// <returns>The formatted string</returns>
    /// <remarks>
    /// Uses the {fmt} library internally.
    /// </remarks>
    template<class... Args>
    inline string format(const string_view fString, const Args&... args) {
        return fmt::format(fmt::runtime(fString), args...);
    }
}

#endif
