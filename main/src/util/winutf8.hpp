// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// UTF-8/UTF-16 conversion helpers for the Windows API

#pragma once

#include <string> // std::string, std::wstring
#include <concepts> // std::integral, std::floating_point

#ifdef _WIN32
// Wide strings are used on Windows
typedef std::wstring widestr;
#else
// Normal strings are used on other platforms
typedef std::string widestr;
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
/// Convert an integer value to a platform-dependent wide string type.
/// </summary>
/// <typeparam name="T">The integer type</typeparam>
/// <param name="val">The integer value</param>
/// <returns>The converted string</returns>
/// <remarks>
/// This template uses C++20 concepts.
/// </remarks>
template <class T>
inline widestr toWide(T val) requires std::integral<T> || std::floating_point<T> {
#ifdef _WIN32
    return std::to_wstring(val);
#else
    return std::to_string(val);
#endif
}
