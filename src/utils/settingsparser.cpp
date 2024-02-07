// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module utils.settingsparser;
import utils.uuids;

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
        static constexpr std::array expectedLengths{ 8, 4, 4, 4, 12 };

        auto [ptrTmp, ec] = std::from_chars(p, data.data() + data.size(), arg, 16);
        bool success = ec == std::errc{};
        bool lengthValid = ptrTmp - p == expectedLengths[lengthIdx++];
        bool delimValid = *ptrTmp == '-' || lengthIdx == 5;

        if (success) arg = UUIDs::byteSwap(arg);

        p = ptrTmp + 1;
        return success && lengthValid && delimValid;
    };

    auto process = [&](auto&... args) { return (extractHex(args) && ...); };

    u32 data1;
    u16 data2;
    u16 data3;
    u16 data4;
    u64 data5;

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
    const auto data1 = UUIDs::byteSwap(*reinterpret_cast<const u32*>(in.data()));
    const auto data2 = UUIDs::byteSwap(*reinterpret_cast<const u16*>(in.data() + 4));
    const auto data3 = UUIDs::byteSwap(*reinterpret_cast<const u16*>(in.data() + 6));
    const auto data4 = UUIDs::byteSwap(*reinterpret_cast<const u64*>(in.data() + 8));

    return std::format("{:08X}-{:04X}-{:04X}-{:04X}-{:012X}", data1, data2, data3, data4 >> 48,
        data4 & 0xFFFFFFFFFFFFULL);
}

void SettingsParser::load(std::string_view filePath) {
    std::ifstream f{ filePath.data() };
    if (!f.is_open()) return;

    std::string line;
    std::string section;
    std::optional<std::string> possibleArrayKey;

    while (std::getline(f, line)) {
        if (trim(line).empty() || line.front() == ';') continue;

        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            possibleArrayKey = std::nullopt;
            continue;
        }

        auto parsed = parse<std::pair<std::string, std::string>>(line);
        bool indented = line.starts_with("  ");
        if (!parsed || indented) {
            if (indented && possibleArrayKey) data[{ section, *possibleArrayKey }] += "\n" + line;

            continue;
        }

        const auto& [key, value] = *parsed;
        data[{ section, key }] = value;

        possibleArrayKey = value.empty() ? std::optional<std::string>{ key } : std::nullopt;
    }
}

void SettingsParser::write(std::string_view filePath) const {
    std::ofstream f{ filePath.data() };

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
