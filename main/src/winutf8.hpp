// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#ifdef _WIN32
typedef std::wstring widestr;

/// <summary>
/// Convert a UTF-8 string into a UTF-16 string.
/// </summary>
/// <param name="from">The input string</param>
/// <returns>The converted output string</returns>
std::wstring toWide(const char* from);

/// <summary>
/// Convert a UTF-16 string into a UTF-8 string.
/// </summary>
/// <param name="from">The input string</param>
/// <returns>The converted output string</returns>
std::string fromWide(const wchar_t* from);
#else
// No wide strings on other platforms, these have no meaning
#define toWide(from) from
#define fromWide(from) from

typedef std::string widestr;
#endif
