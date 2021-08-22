// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// String utilities that are not present in the C++ Standard

#pragma once

#include <string> // std::string

namespace StringUtils {
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
	std::string replaceAll(const std::string& str, const std::string& from, const std::string& to);
}
