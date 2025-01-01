// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

#include <imgui.h>

namespace ImGuiExt {
    // Dimension expressed in terms of font size, for DPI-aware size calculations.
    struct Dimension {
        float s; // Size

        // Sets the size of this dimension.
        template <class T>
        requires std::integral<T> || std::floating_point<T>
        constexpr explicit Dimension(T s) : s(static_cast<float>(s) * ImGui::GetFontSize()) {}

        // Gets the size of this dimension. This is not explicit so dimensions can be treated exactly like floats.
        constexpr explicit(false) operator float() const {
            return s;
        }

        // Combines this dimension with another to create an ImVec2. For use with 2D sizes.
        constexpr ImVec2 operator*(Dimension other) const {
            return { *this, other };
        }
    };

    namespace Literals {
        // Multiplies the given value by the font height. For use with calculating item sizes.
        inline Dimension operator""_fh(long double s) {
            return Dimension{ s };
        }

        inline Dimension operator""_fh(unsigned long long int s) {
            return Dimension{ s };
        }

        // Multiplies the given value by DeltaTime. For use with consistent transitions and movement.
        inline float operator""_dt(long double s) {
            return static_cast<float>(s) * ImGui::GetIO().DeltaTime;
        }

        inline float operator""_dt(unsigned long long s) {
            return static_cast<float>(s) * ImGui::GetIO().DeltaTime;
        }
    }

    constexpr float fill = -std::numeric_limits<float>::min(); // Makes a widget fill a dimension. Use with ImVec2.

    // Wrapper for TextUnformatted() to allow a string_view parameter.
    inline void textUnformatted(std::string_view s) {
        ImGui::TextUnformatted(s.data());
    }

    // Wrapper for RadioButton() to control a variable and its value.
    template <class T>
    void radioButton(std::string_view label, T& var, T value) {
        if (ImGui::RadioButton(label.data(), var == value)) var = value;
    }

    // Gets the ImGuiDataType of a given variable.
    template <class T>
    constexpr ImGuiDataType getDataType();

    // Wrapper for InputScalar() with automatic type detection.
    template <class T>
    void inputScalar(std::string_view label, T& data, int step = 0, int stepFast = 0, const char* format = nullptr) {
        // Any negative step value is considered invalid and nullptr is passed to disable the step buttons
        int* stepPtr = step > 0 ? &step : nullptr;
        int* stepFastPtr = step > 0 && stepFast > 0 ? &stepFast : nullptr;

        ImGui::InputScalar(label.data(), getDataType<T>(), &data, stepPtr, stepFastPtr, format);
    }

    // Wrapper for InputText() to use a std::string buffer.
    bool inputText(std::string_view label, std::string& s, ImGuiInputTextFlags flags = 0);

    // Wrapper for InputTextMultiline() to use a std::string buffer.
    bool inputTextMultiline(std::string_view label, std::string& s, const ImVec2& size = {},
        ImGuiInputTextFlags flags = 0);

    // Creates a (?) mark which shows a tooltip on hover.
    // This can be placed next to a widget to provide more details about it.
    void helpMarker(std::string_view desc);

    // Displays a basic spinner which rotates every few frames.
    void spinner();

    // Displays a menu item that focuses on a window when clicked.
    inline void windowMenuItem(std::string_view name) {
        if (ImGui::MenuItem(name.data())) ImGui::SetWindowFocus(name.data());
    }

    // Constructs a shortcut string according to the platform.
    constexpr std::array<char, 7> shortcut(char key) {
        // macOS: U+EBB8 (Cmd symbol in Remix Icon)
        // Other platforms: Ctrl+[key]
        if constexpr (OS_MACOS) return { '\xee', '\xae', '\xb8', key, '\0' };
        else return { 'C', 't', 'r', 'l', '+', key, '\0' };
    }
}

template <class T>
constexpr ImGuiDataType ImGuiExt::getDataType() {
    if constexpr (std::is_same_v<T, std::int8_t>) return ImGuiDataType_S8;
    if constexpr (std::is_same_v<T, std::uint8_t>) return ImGuiDataType_U8;
    if constexpr (std::is_same_v<T, std::int16_t>) return ImGuiDataType_S16;
    if constexpr (std::is_same_v<T, std::uint16_t>) return ImGuiDataType_U16;
    if constexpr (std::is_same_v<T, std::int32_t>) return ImGuiDataType_S32;
    if constexpr (std::is_same_v<T, std::uint32_t>) return ImGuiDataType_U32;
    if constexpr (std::is_same_v<T, std::int64_t>) return ImGuiDataType_S64;
    if constexpr (std::is_same_v<T, std::uint64_t>) return ImGuiDataType_U64;
    if constexpr (std::is_same_v<T, float>) return ImGuiDataType_Float;
    if constexpr (std::is_same_v<T, double>) return ImGuiDataType_Double;
}
