#ifdef _WIN32
#include <Windows.h>
#endif

#include "strings.hpp"

Strings::widestr Strings::toWide(const std::string& from) {
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

std::string Strings::fromWide(const Strings::widestr& from) {
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

std::string Strings::replaceAll(NO_CONST_REF std::string str, const std::string& from, const std::string& to) {
    // Preliminary checks
    // 1. Nothing to replace in an empty string
    // 2. Can't replace an empty string
    // 3. If from and to are equal the function call becomes pointless
    if (str.empty() || from.empty() || (from == to)) return str;

    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }

    return str;
}

std::vector<std::string> Strings::split(NO_CONST_REF std::string str, char delim) {
    // Return value
    std::vector<std::string> ret;

    // Find each delimited substring in the buffer to extract each individual substring
    size_t pos = 0;
    while ((pos = str.find(delim)) != std::string::npos) {
        pos++; // Increment position to include the delimiter in the substring
        ret.push_back(str.substr(0, pos)); // Get the substring, add it to the vector
        str.erase(0, pos); // Erase the substring from the buffer
    }

    // Add the last substring (all other substrings have been erased from the buffer, the only one left is the one after
    // the last delimiter char, or the whole string if there are no \n's present)
    ret.push_back(str);
    return ret;
}
