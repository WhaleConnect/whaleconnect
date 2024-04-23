// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>
#include <string_view>

#if OS_WINDOWS
#include <Windows.h>
#endif

#include "strings.hpp"

Strings::SysStr Strings::toSys(std::string_view from) {
#if OS_WINDOWS
    // Nothing to convert in an empty string
    if (from.empty()) return {};

    // Size of UTF-8 string in UTF-16 wide encoding
    int stringSize = MultiByteToWideChar(CP_UTF8, 0, from.data(), static_cast<int>(from.size()), nullptr, 0);

    // Buffer to contain new string
    std::wstring buf(stringSize, '\0');

    // Convert the string from UTF-8 and return the converted buffer
    MultiByteToWideChar(CP_UTF8, 0, from.data(), -1, buf.data(), stringSize);
    return buf;
#else
    return Strings::SysStr{ from };
#endif
}

std::string Strings::fromSys(SysStrView from) {
#if OS_WINDOWS
    // Nothing to convert in an empty string
    if (from.empty()) return {};

    // Size of UTF-16 wide string in UTF-8 encoding
    int stringSize
        = WideCharToMultiByte(CP_UTF8, 0, from.data(), static_cast<int>(from.size()), nullptr, 0, nullptr, nullptr);

    // Buffer to contain new string
    std::string buf(stringSize, '\0');

    // Convert the string to UTF-8 and return the converted buffer
    WideCharToMultiByte(CP_UTF8, 0, from.data(), -1, buf.data(), stringSize, nullptr, nullptr);
    return buf;
#else
    return std::string{ from };
#endif
}

std::string Strings::replaceAll(std::string str, std::string_view from, std::string_view to) {
    // Adapted from https://stackoverflow.com/a/3418285

    // Preliminary checks
    // 1. Nothing to replace in an empty string
    // 2. Can't replace an empty string
    // 3. If from and to are equal the function call becomes pointless
    if (str.empty() || from.empty() || from == to) return str;

    std::size_t start = 0;
    while ((start = str.find(from, start)) != std::string::npos) {
        str.replace(start, from.size(), to);
        start += to.size(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }

    return str;
}
