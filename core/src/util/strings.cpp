// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#include <Windows.h>
#endif

#include "strings.hpp"

Strings::WideStr Strings::toWide(std::string_view from) {
#ifdef _WIN32
    // Nothing to convert in an empty string
    if (from.empty()) return {};

    // Size of UTF-8 string in UTF-16 wide encoding
    int stringSize = MultiByteToWideChar(CP_UTF8, 0, from.data(), static_cast<int>(from.size()), 0, 0);

    // Buffer to contain new string
    std::wstring buf(stringSize, '\0');

    // Convert the string from UTF-8 and return the converted buffer
    MultiByteToWideChar(CP_UTF8, 0, from.data(), -1, buf.data(), stringSize);
    return buf;
#else
    return from;
#endif
}

std::string Strings::fromWide(std::wstring_view from) {
#ifdef _WIN32
    // Nothing to convert in an empty string
    if (from.empty()) return {};

    // Size of UTF-16 wide string in UTF-8 encoding
    int stringSize = WideCharToMultiByte(CP_UTF8, 0, from.data(), static_cast<int>(from.size()), 0, 0, 0, 0);

    // Buffer to contain new string
    std::string buf(stringSize, '\0');

    // Convert the string to UTF-8 and return the converted buffer
    WideCharToMultiByte(CP_UTF8, 0, from.data(), -1, buf.data(), stringSize, nullptr, nullptr);
    return buf;
#else
    return from;
#endif
}

std::string Strings::replaceAll(NO_CONST_REF std::string str, std::string_view from, std::string_view to) {
    // Preliminary checks
    // 1. Nothing to replace in an empty string
    // 2. Can't replace an empty string
    // 3. If from and to are equal the function call becomes pointless
    if (str.empty() || from.empty() || (from == to)) return str;

    size_t start = 0;
    while ((start = str.find(from, start)) != std::string::npos) {
        str.replace(start, from.size(), to);
        start += to.size(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }

    return str;
}

std::vector<std::string_view> Strings::split(std::string_view str, char delim) {
    // Return value
    std::vector<std::string_view> ret;

    // Find each delimited substring in the buffer to extract each individual substring

    // Index of the first delimiter found
    size_t first = 0;

    while (first < str.size()) {
        // Find the index of the next delimiter after the last one found
        size_t next = str.find_first_of(delim, first);

        // Add the substring between the two delimiters
        ret.push_back(str.substr(first, next - first));

        // If there are no more occurrences of the delimiters we can exit
        if (next == std::string_view::npos) break;

        // Go past the last one found and continue
        first = next + 1;
    }

    return ret;
}
