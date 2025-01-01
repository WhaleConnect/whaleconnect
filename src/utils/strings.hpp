// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>
#include <type_traits>

namespace Strings {
    // Generalized string type for system functions.
    //
    // On Windows, for a program to be Unicode-aware, it needs to use the Windows API functions ending in "W",
    // indicating the function takes strings of wchar_t which are UTF-16 encoded.
    // Other platforms can use strings of char which are UTF-8 encoded and can handle Unicode.
    using SysStr = std::conditional_t<OS_WINDOWS, std::wstring, std::string>;

    // Generalized string view type for system functions.
    using SysStrView = std::basic_string_view<SysStr::value_type>;

    // Converts a UTF-8 string into a UTF-16 string on Windows.
    SysStr toSys(std::string_view from);

    // Converts a UTF-16 string into a UTF-8 string on Windows.
    std::string fromSys(SysStrView from);

    // Converts an integer or decimal value to a system string.
    template <class T>
    requires std::integral<T> || std::floating_point<T>
    SysStr toSys(T from) {
        if constexpr (OS_WINDOWS) return std::to_wstring(from);
        else return std::to_string(from);
    }

    // Replaces all occurrences of a substring within a given base string.
    std::string replaceAll(std::string str, std::string_view from, std::string_view to);

    // Removes null characters from the end of a string.
    template <class T>
    void stripNull(std::basic_string<T>& s) {
        s.erase(s.find(T{}));
    }
}
