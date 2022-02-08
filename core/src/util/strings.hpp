// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// String utilities that are not present in the C++ Standard

#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <concepts> // std::integral, std::floating_point

// Explicitly annotate parameters passed by value, not by const-qualified reference, which does not elide copies.
// Creating a copy is intentional (not a mistake) when a parameter is prefixed with this macro.
#define NO_CONST_REF

/// <summary>
/// Namespace containing general utility functions for string manipuation.
/// </summary>
namespace Strings {
#ifdef _WIN32
    // Wide strings are used on Windows
    using WideStr = std::wstring;
#else
    // Standard strings are used on other platforms
    using WideStr = std::string;
#endif

    /// <summary>
    /// Convert a UTF-8 string into a UTF-16 string.
    /// </summary>
    /// <param name="from">The input string</param>
    /// <returns>The converted output string</returns>
    WideStr toWide(std::string_view from);

    /// <summary>
    /// Convert a UTF-16 string into a UTF-8 string.
    /// </summary>
    /// <param name="from">The input string</param>
    /// <returns>The converted output string</returns>
    std::string fromWide(std::wstring_view from);

    /// <summary>
    /// Convert an integer or decimal value to a platform-dependent wide string type.
    /// </summary>
    /// <typeparam name="T">A numeric type (e.g. int, long, double)</typeparam>
    /// <param name="val">The numeric value to convert</param>
    /// <returns>The converted string</returns>
    template <class T>
    inline WideStr toWide(T val) requires std::integral<T> || std::floating_point<T> {
#ifdef _WIN32
        return std::to_wstring(val);
#else
        return std::to_string(val);
#endif
    }

    /// <summary>
    /// Replace all occurrences of a substring within a given base string.
    /// </summary>
    /// <param name="str">The base string</param>
    /// <param name="from">The substring to replace</param>
    /// <param name="to">What to replace the substring with</param>
    /// <returns>The original string with all occurrences of `from` replaced with `to`</returns>
    /// <remarks>
    /// This function is adapted from https://stackoverflow.com/a/3418285
    /// </remarks>
    std::string replaceAll(NO_CONST_REF std::string str, std::string_view from, std::string_view to);
}
