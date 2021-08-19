// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// UTF-8/UTF-16 conversion helpers for the Windows API

#pragma once

#include <string> // std::string

#ifdef _WIN32
// Convert an integer type to a wstring.
// std::to_(w)string() has too many overloads to make a proper function wrapper.
// So use a macro to do so instead
#define I_TO_WIDE std::to_wstring

typedef std::wstring widestr;

/// <summary>
/// Convert a UTF-8 string into a UTF-16 string.
/// </summary>
/// <param name="from">The input string</param>
/// <returns>The converted output string</returns>
std::wstring toWide(const std::string& from);

/// <summary>
/// Convert a UTF-16 string into a UTF-8 string.
/// </summary>
/// <param name="from">The input string</param>
/// <returns>The converted output string</returns>
std::string fromWide(const std::wstring& from);
#else
// No wide strings on other platforms
#define I_TO_WIDE std::to_string
#define toWide(from) from
#define fromWide(from) from

typedef std::string widestr;
#endif
