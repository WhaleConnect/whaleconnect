// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// The naming conventions used in this file are selected to remain consistent with the official Dear ImGui API.

#pragma once

#include <cfloat>
#include <string>
#include <string_view>
#include <type_traits> // std::is_same_v

#include <imgui.h>

namespace ImGui {
    constexpr float FILL = -FLT_MIN; // Makes a widget fill a dimension. Use with ImVec2.

    // A wrapper for TextUnformatted() to allow a string_view parameter.
    inline void TextUnformatted(std::string_view s) { TextUnformatted(s.data()); }

    // Wrapper for RadioButton() to control a variable and its value.
    template <class T>
    void RadioButton(std::string_view label, T& var, T value) {
        if (RadioButton(label.data(), var == value)) var = value;
    }

    // Gets the width of a rendered string added with the item inner spacing specified in the Dear ImGui style.
    inline float CalcTextWidthWithSpacing(std::string_view text) {
        return GetStyle().ItemInnerSpacing.x + CalcTextSize(text.data()).x;
    }

    // Gets the ImGuiDataType of a given variable.
    template <class T>
    constexpr ImGuiDataType GetDataType(T);

    // Wrapper for InputScalar() with automatic type detection.
    template <class T, class U = T>
    void InputScalar(std::string_view label, T& data, U step = 0, U stepFast = 0) {
        // Any negative step value is considered invalid and nullptr is passed to disable the step buttons
        U* stepPtr = (step > 0) ? &step : nullptr;
        U* stepFastPtr = ((step > 0) && (stepFast > 0)) ? &stepFast : nullptr;

        InputScalar(label.data(), GetDataType(data), &data, stepPtr, stepFastPtr);
    }

    // Wrapper for InputText() to use a std::string buffer.
    bool InputText(std::string_view label, std::string& s, ImGuiInputTextFlags flags = 0);

    // Wrapper for InputTextMultiline() to use a std::string buffer.
    bool InputTextMultiline(std::string_view label, std::string& s, const ImVec2& size = {},
                            ImGuiInputTextFlags flags = 0);

    // Creates a (?) mark which shows a tooltip on hover.
    // This can be placed next to a widget to provide more details about it.
    void HelpMarker(std::string_view desc);

    // Displays a basic spinner which rotates every few frames.
    void Spinner();
}

template <class T>
constexpr ImGuiDataType ImGui::GetDataType(T) {
    if constexpr (std::is_same_v<T, int8_t>) return ImGuiDataType_S8;
    if constexpr (std::is_same_v<T, uint8_t>) return ImGuiDataType_U8;
    if constexpr (std::is_same_v<T, int16_t>) return ImGuiDataType_S16;
    if constexpr (std::is_same_v<T, uint16_t>) return ImGuiDataType_U16;
    if constexpr (std::is_same_v<T, int32_t>) return ImGuiDataType_S32;
    if constexpr (std::is_same_v<T, uint32_t>) return ImGuiDataType_U32;
    if constexpr (std::is_same_v<T, int64_t>) return ImGuiDataType_S64;
    if constexpr (std::is_same_v<T, uint64_t>) return ImGuiDataType_U64;
    if constexpr (std::is_same_v<T, float>) return ImGuiDataType_Float;
    if constexpr (std::is_same_v<T, double>) return ImGuiDataType_Double;
}
