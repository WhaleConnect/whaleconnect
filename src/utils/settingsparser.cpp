// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settingsparser.hpp"

#include <charconv>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "uuids.hpp"

std::string trim(std::string_view s) {
    std::size_t first = s.find_first_not_of(" ");
    std::size_t last = s.find_last_not_of(" ");

    std::size_t start = first == std::string::npos ? 0 : first;
    std::size_t end = last == std::string::npos ? 0 : last - start + 1;

    return std::string{ s.substr(start, end) };
}

template <>
ParseResult<UUIDs::UUID128> parse(std::string_view data) {
    const char* p = data.data();
    int lengthIdx = 0;

    auto extractHex = [&](auto& arg) {
        // Lengths of UUID segments in characters
        static constexpr std::array expectedLengths{ 8, 4, 4, 4, 12 };

        auto [ptrTmp, ec] = std::from_chars(p, data.data() + data.size(), arg, 16);
        bool success = ec == std::errc{};
        bool lengthValid = ptrTmp - p == expectedLengths[lengthIdx++];
        bool delimValid = *ptrTmp == '-' || lengthIdx == 5; // At delimiter or end of string

        if (success) arg = UUIDs::byteSwap(arg);

        p = ptrTmp + 1;
        return success && lengthValid && delimValid;
    };

    auto process = [&](auto&... args) { return (extractHex(args) && ...); };

    std::uint32_t data1;
    std::uint16_t data2;
    std::uint16_t data3;
    std::uint16_t data4;
    std::uint64_t data5;

    UUIDs::UUID128 ret;
    if (process(data1, data2, data3, data4, data5)) {
        std::memcpy(ret.data(), &data1, 4);
        std::memcpy(ret.data() + 4, &data2, 2);
        std::memcpy(ret.data() + 6, &data3, 2);
        std::memcpy(ret.data() + 8, &data4, 2);
        std::memcpy(ret.data() + 10, reinterpret_cast<char*>(&data5) + 2, 6);
        return ret;
    }
    return std::nullopt;
}

template <>
std::string stringify(const UUIDs::UUID128& in) {
    const auto data1 = UUIDs::byteSwap(*reinterpret_cast<const std::uint32_t*>(in.data()));
    const auto data2 = UUIDs::byteSwap(*reinterpret_cast<const std::uint16_t*>(in.data() + 4));
    const auto data3 = UUIDs::byteSwap(*reinterpret_cast<const std::uint16_t*>(in.data() + 6));
    const auto data4 = UUIDs::byteSwap(*reinterpret_cast<const std::uint64_t*>(in.data() + 8));

    return std::format("{:08X}-{:04X}-{:04X}-{:04X}-{:012X}", data1, data2, data3, data4 >> 48,
        data4 & 0xFFFFFFFFFFFFULL);
}

void SettingsParser::load(const std::filesystem::path& filePath) {
    std::ifstream f{ filePath };
    if (!f.is_open()) return;

    std::string line;
    std::string section;
    std::optional<std::string> possibleArrayKey;

    while (std::getline(f, line)) {
        // Ignore comments and empty lines
        if (trim(line).empty() || line.front() == ';') continue;

        // Extract section name
        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            possibleArrayKey = std::nullopt;
            continue;
        }

        // Parse each line as a key:string = value:string pair
        auto parsed = parse<std::pair<std::string, std::string>>(line);
        bool indented = line.starts_with("  ");
        if (!parsed || indented) {
            // Add indented lines to an array
            if (indented && possibleArrayKey) data[{ section, *possibleArrayKey }] += "\n" + line;

            // Skip invalid parses
            continue;
        }

        const auto& [key, value] = *parsed;
        data[{ section, key }] = value;

        // If there is no value, it is possible the next line starts an array
        possibleArrayKey = value.empty() ? std::optional<std::string>{ key } : std::nullopt;
    }
}

void SettingsParser::write(const std::filesystem::path& filePath) const {
    std::ofstream f{ filePath };

    std::string currentKey;
    for (const auto& [keys, value] : data) {
        const auto& [section, key] = keys;
        if (section != currentKey) {
            f << "[" << section << "]\n";
            currentKey = section;
        }

        f << stringify(std::pair{ key, value }) << "\n";
    }
}
