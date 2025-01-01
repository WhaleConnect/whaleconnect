// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <charconv>
#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "uuids.hpp"

// Type traits

template <class T>
concept Integer = std::integral<T> && (!std::same_as<T, bool>);

template <class, template <class...> class>
constexpr bool isSpecialization = false;

template <template <class...> class T, class... Args>
constexpr bool isSpecialization<T<Args...>, T> = true;

template <class T>
concept Vector = isSpecialization<T, std::vector>;

template <class T>
concept Pair = isSpecialization<T, std::pair>;

template <class T>
concept Map = isSpecialization<T, std::map>;

// Parsing (string -> data)

template <class T>
using ParseResult = std::optional<T>;

std::string trim(std::string_view s);

template <class T>
ParseResult<T> parse(std::string_view) {
    std::unreachable();
}

template <>
inline ParseResult<std::string> parse(std::string_view data) {
    return std::string{ data };
}

template <>
inline ParseResult<bool> parse(std::string_view data) {
    if (data == "true") return true;
    else if (data == "false") return false;
    return std::nullopt;
}

template <Pair T>
ParseResult<T> parse(std::string_view data) {
    std::size_t equalIdx = data.find('=');
    if (equalIdx == std::string::npos) return std::nullopt;

    std::string key = trim(data.substr(0, equalIdx));
    std::string value = trim(data.substr(equalIdx + 1));

    auto valueParsed = parse<typename T::second_type>(value);
    if (!valueParsed) return std::nullopt;
    return ParseResult<T>{ std::in_place, key, *valueParsed };
}

template <>
ParseResult<UUIDs::UUID128> parse(std::string_view data);

template <Integer T>
ParseResult<T> parse(std::string_view data) {
    T ret;
    auto [_, ec] = std::from_chars(data.data(), data.data() + data.size(), ret);
    return ec == std::errc{} ? ParseResult<T>{ ret } : std::nullopt;
}

template <Vector T>
ParseResult<T> parse(std::string_view data) {
    using namespace std::literals;
    const auto transform = [](const auto& i) { return parse<typename T::value_type>(std::string_view{ i }).value(); };

    try {
        return data | std::views::split("\n  "sv) | std::views::drop(1) | std::views::transform(transform)
            | std::ranges::to<T>();
    } catch (const std::bad_optional_access&) {
        return std::nullopt;
    }
}

template <Map T>
ParseResult<T> parse(std::string_view data) {
    auto result = parse<std::vector<std::pair<std::string, typename T::mapped_type>>>(data);
    if (!result) return std::nullopt;

    return *result | std::ranges::to<T>();
}

// Stringification (data -> string)
// All types are passed as const& to work with template specialization.

template <class T>
std::string stringify(const T&) {
    std::unreachable();
}

template <>
inline std::string stringify(const std::string& in) {
    return in;
}

template <Integer T>
std::string stringify(const T& in) {
    return std::to_string(in);
}

template <>
inline std::string stringify(const bool& in) {
    return in ? "true" : "false";
}

template <Pair T>
std::string stringify(const T& in) {
    return std::format("{} = {}", in.first, stringify(in.second));
}

template <>
std::string stringify(const UUIDs::UUID128& in);

template <Vector T>
std::string stringify(const T& in) {
    const auto transform = [](const auto& i) { return "\n  " + stringify(i); };
    return in | std::views::transform(transform) | std::views::join | std::ranges::to<std::string>();
}

template <Map T>
std::string stringify(const T& in) {
    return stringify(in | std::ranges::to<std::vector>());
}

// Class to parse settings from an ini file.
// Supported data types: string, integer, boolean, UUID, array, map
// Lines starting with a semicolon are treated as comments
class SettingsParser {
    using Data = std::map<std::pair<std::string, std::string>, std::string>;
    Data data;

public:
    void load(const std::filesystem::path& filePath);

    template <class T>
    T get(const std::string& section, const std::string& key, const T& defaultValue = {}) const {
        if (Data::key_type mapKey{ section, key }; data.contains(mapKey))
            return parse<T>(data.at(mapKey)).value_or(defaultValue);
        else return defaultValue;
    }

    template <class T>
    void set(const std::string& section, const std::string& key, const T& in) {
        data[{ section, key }] = stringify(in);
    }

    void write(const std::filesystem::path& filePath) const;
};
