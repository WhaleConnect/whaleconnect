// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief String utilities that are not present in the C++ Standard
*/

#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <concepts> // std::integral, std::floating_point

namespace Strings {
    /**
     * @brief A generalized string type for system functions.
     *
     * On Windows, for a program to be Unicode-aware, it needs to use the Windows API functions ending in "W",
     * indicating the function takes strings of @p wchar_t which are UTF-16 encoded.<br>
     * Other platforms can use strings of @p char which are UTF-8 encoded and can handle Unicode.<br>
    */
    using SysStr =
#ifdef _WIN32
        std::wstring
#else
        std::string
#endif
        ;

    /**
     * @brief A generalized string view type for system functions.
    */
    using SysStrView = std::basic_string_view<SysStr::value_type>;

    /**
     * @brief Converts a UTF-8 string into a UTF-16 string on Windows.
     * @param from The input string
     * @return The converted string
    */
    SysStr toSys(std::string_view from);

    /**
     * @brief Converts a UTF-16 string into a UTF-8 string on Windows.
     * @param from The input string
     * @return The converted string
    */
    std::string fromSys(SysStrView from);

    /**
     * @brief Converts an integer or decimal value to a system string.
     * @tparam T A numeric type
     * @param from The input numeric value
     * @return The converted string
    */
    template <class T>
    inline SysStr toSys(T from) requires std::integral<T> || std::floating_point<T> {
#ifdef _WIN32
        return std::to_wstring(from);
#else
        return std::to_string(from);
#endif
    }

    /**
     * @brief Replaces all occurrences of a substring within a given base string.
     * @param str The base string
     * @param from The substring to replace
     * @param to What to replace the substring with
     * @return The original string with all occurrences of @p from replaced with @p to
    */
    std::string replaceAll(std::string str, std::string_view from, std::string_view to);
}
