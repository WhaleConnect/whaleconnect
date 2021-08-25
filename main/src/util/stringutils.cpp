#include "stringutils.hpp"

std::string StringUtils::replaceAll(std::string str, const std::string& from, const std::string& to) {
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
