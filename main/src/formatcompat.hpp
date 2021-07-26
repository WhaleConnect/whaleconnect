// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// TODO: Remove this fallback file and just use `#include <format>` when all major compilers implement it

#pragma once

// We can detect the presence of headers with __has_include(), but make sure it's defined first.
#ifdef __has_include
#if __has_include(<format>)
// #include <format> exists, directly use it
#include <format> // std::format() [C++20]
#else
// Standard headers needed for format compatibility
#include <string>
#include <string_view>

// #include <format> doesn't exist, use {fmt} as a fallback:
#include <fmt/core.h>

namespace std {
    /// <summary>
    /// Compatibility layer for C++20's std::format. Uses the {fmt} library internally.
    /// </summary>
    /// <typeparam name="...Args">Variadic arguments to format into a given string</typeparam>
    /// <param name="fString">The string to format with formatting templates</param>
    /// <param name="...args">Arguments to format into the string</param>
    /// <returns>The formatted string</returns>
    template<class... Args>
    inline string format(const string_view fString, const Args&... args) {
        return fmt::format(fString, args...);
    }
}

#endif
#endif
