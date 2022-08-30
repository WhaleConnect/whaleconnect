// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief Compatibility layer for C++20 @p std::format() using the {fmt} library internally
*/

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
    /**
     * @brief Polyfill for @p std::format().
     * @tparam ...Args A sequence of arguments to format
     * @param fString The format string
     * @param ...args Arguments to format into the string
     * @return The formatted string
    */
    template <class... Args>
    inline string format(string_view fString, Args&&... args) {
        return fmt::format(fmt::runtime(fString), forward<Args>(args)...);
    }

    /**
     * @brief Polyfill for @p std::format_to().
     * @tparam OutputIt An iterator type
     * @tparam ...Args A sequence of arguments to format
     * @param out An iterator to the output buffer to format into
     * @param fString The format string
     * @param ...args Arguments to format into the string
     * @return An iterator past the end of the output range
    */
    template <class OutputIt, class... Args>
    inline OutputIt format_to(OutputIt out, string_view fString, Args&&... args) {
        return fmt::format_to(out, fmt::runtime(fString), forward<Args>(args)...);
    }
}

#endif
