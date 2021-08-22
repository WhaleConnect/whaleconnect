#include "stringutils.hpp"

std::string StringUtils::replaceAll(const std::string& str, const std::string& from, const std::string& to) {
    std::string ret = str;

    // Can't replace an empty string
    if (from.empty()) return ret;

    // If from and to are equal the function call becomes pointless
    if (from == to) return ret;

    size_t start_pos = 0;
    while ((start_pos = ret.find(from, start_pos)) != std::string::npos) {
        ret.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }

    return ret;
}
