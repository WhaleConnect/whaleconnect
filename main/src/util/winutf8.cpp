// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#include <Windows.h>
#endif

#include "winutf8.hpp"

widestr toWide(const std::string& from) {
#ifdef _WIN32
    // Size of UTF-8 string in UTF-16 wide encoding
    int stringSize = MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, nullptr, 0);

    // Allocate char buffer to contain new string
    wchar_t* tmpBuf = new wchar_t[stringSize];

    // Convert the string from UTF-8 and store it in the buffer
    MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, tmpBuf, stringSize);

    // Copy buffer into string object, free original buffer, and return
    widestr ret = tmpBuf;
    delete[] tmpBuf;
    return ret;
#else
    return from;
#endif
}

std::string fromWide(const widestr& from) {
#ifdef _WIN32
    // Size of UTF-16 wide string in UTF-8 encoding
    int stringSize = WideCharToMultiByte(CP_UTF8, 0, from.c_str(), -1, 0, 0, nullptr, nullptr);

    // Allocate char buffer to contain new string
    char* tmpBuf = new char[stringSize];

    // Convert the string to UTF-8 and store it in the buffer
    WideCharToMultiByte(CP_UTF8, 0, from.c_str(), -1, tmpBuf, stringSize, nullptr, nullptr);

    // Copy buffer into string object, free original buffer, and return
    std::string ret = tmpBuf;
    delete[] tmpBuf;
    return ret;
#else
    return from;
#endif
}
