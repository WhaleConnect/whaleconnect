// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifdef _WIN32
#include <string>

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
#endif
