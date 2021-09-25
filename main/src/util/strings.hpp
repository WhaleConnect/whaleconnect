// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// String utilities that are not present in the C++ Standard

#pragma once

#include <string> // std::string
#include <vector> // std::vector
#include <concepts> // std::integral, std::floating_point [C++20]

// Explicitly annotate parameters passed by value, not by const-qualified reference, which does not elide copies.
// Creating a copy is intentional (not a mistake) when a parameter is prefixed with this macro.
#define NO_CONST_REF

/// <summary>
/// Namespace containing general utility functions for string manipuation.
/// </summary>
namespace Strings {
#ifdef _WIN32
    // Wide strings are used on Windows
    using widestr = std::wstring;
#else
    // Standard strings are used on other platforms
    using widestr = std::string;
#endif

    /// <summary>
    /// Convert a UTF-8 string into a UTF-16 string.
    /// </summary>
    /// <param name="from">The input string</param>
    /// <returns>The converted output string</returns>
    widestr toWide(const std::string& from);

    /// <summary>
    /// Convert a UTF-16 string into a UTF-8 string.
    /// </summary>
    /// <param name="from">The input string</param>
    /// <returns>The converted output string</returns>
    std::string fromWide(const widestr& from);

    /// <summary>
    /// Convert an integer or decimal value to a platform-dependent wide string type.
    /// </summary>
    /// <typeparam name="T">A numeric type (e.g. int, long, double)</typeparam>
    /// <param name="val">The numeric value to convert</param>
    /// <returns>The converted string</returns>
    template <class T>
    inline widestr toWide(T val) requires std::integral<T> || std::floating_point<T> {
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
    std::string replaceAll(NO_CONST_REF std::string str, const std::string& from, const std::string& to);

    /// <summary>
    /// Split a string into substrings given a delimiter char.
    /// </summary>
    /// <param name="str">The string to split</param>
    /// <param name="delim">The delimiter character to split the string with</param>
    /// <returns>A vector containing each substring</returns>
    std::vector<std::string> split(NO_CONST_REF std::string str, char delim);
}
