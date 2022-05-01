// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Compatibility layer for C++20 std::format() using the {fmt} library internally.

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
    /// Polyfill function for `std::format()`.
    /// </summary>
    /// <typeparam name="...Args">Variadic arguments to format</typeparam>
    /// <param name="fString">The format string</param>
    /// <param name="...args">Arguments to format into the string</param>
    /// <returns>The formatted string</returns>
    template <class... Args>
    inline string format(string_view fString, Args&&... args) {
        return fmt::format(fmt::runtime(fString), forward<Args>(args)...);
    }

    /// <summary>
    /// Polyfill function for `std::format_to()`.
    /// </summary>
    /// <typeparam name="OutputIt">An output iterator</typeparam>
    /// <typeparam name="...Args">Variadic arguments to format</typeparam>
    /// <param name="out">An iterator to the output buffer</param>
    /// <param name="fString">The format string</param>
    /// <param name="...args">Arguments to format into the string</param>
    /// <returns>An iterator past the end of the output range</returns>
    template <class OutputIt, class... Args>
    inline OutputIt format_to(OutputIt out, string_view fString, Args&&... args) {
        return fmt::format_to(out, fmt::runtime(fString), forward<Args>(args)...);
    }
}

#endif
