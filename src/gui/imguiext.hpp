// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// The naming conventions used in this file are selected to remain consistent with the official Dear ImGui API.

#pragma once

#include <cfloat>
#include <string>
#include <string_view>
#include <type_traits> // std::is_same_v

#include <imgui.h>

// The corners of the application window where an overlay can be drawn.
enum class ImGuiOverlayCorner { TopLeft, TopRight, BottomLeft, BottomRight };

namespace ImGui {
    constexpr float FILL = -FLT_MIN; // Makes a widget fill a dimension. Use with ImVec2.

    // A wrapper for TextUnformatted() to allow a string_view parameter.
    inline void TextUnformatted(std::string_view s) { TextUnformatted(s.data()); }

    // A wrapper for RadioButton() to control a variable and its value.
    template <class T>
    void RadioButton(const char* label, T& var, T value) {
        if (RadioButton(label, var == value)) var = value;
    }

    // Begins a child window with space at the bottom.
    inline void BeginChildSpacing(const char* id, float space, bool border = false, ImGuiWindowFlags flags = 0) {
        BeginChild(id, { 0, space * -ImGui::GetFrameHeightWithSpacing() }, border, flags);
    }

    // Gets the width of a rendered string added with the item inner spacing specified in the Dear ImGui style.
    inline float CalcTextWidthWithSpacing(const char* text) {
        return GetStyle().ItemInnerSpacing.x + CalcTextSize(text).x;
    }

    // Gets the ImGuiDataType of a given variable.
    template <class T>
    constexpr ImGuiDataType GetDataType(T);

    // A wrapper for InputScalar() with automatic type detection.
    template <class T, class U = int>
    inline void InputScalar(const char* label, T& data, U step = 0, U stepFast = 0) {
        // Any negative step value is considered invalid and nullptr is passed to disable the step buttons
        U* stepPtr = (step > 0) ? &step : nullptr;
        U* stepFastPtr = ((step > 0) && (stepFast > 0)) ? &stepFast : nullptr;

        InputScalar(label, GetDataType(data), &data, stepPtr, stepFastPtr);
    }

    // A wrapper for InputText() to use a std::string buffer.
    bool InputText(const char* label, std::string& s, ImGuiInputTextFlags flags = 0);

    // A wrapper for InputTextMultiline() to use a std::string buffer.
    bool InputTextMultiline(const char* label, std::string& s, const ImVec2& size = {}, ImGuiInputTextFlags flags = 0);

    // Creates a (?) mark which shows a tooltip on hover.
    // This can be placed next to a widget to provide more details about it.
    void HelpMarker(const char* desc);

    // Creates a semi-transparent, fixed overlay on the application window.
    template <class... Args>
    void Overlay(const ImVec2& padding, ImGuiOverlayCorner corner, const char* text, Args&&... args);

    // Displays a basic spinner which rotates every few frames.
    void Spinner();
}

template <class T>
constexpr ImGuiDataType ImGui::GetDataType(T) {
    if constexpr (std::is_same_v<T, int8_t>) return ImGuiDataType_S8;
    else if constexpr (std::is_same_v<T, uint8_t>) return ImGuiDataType_U8;
    else if constexpr (std::is_same_v<T, int16_t>) return ImGuiDataType_S16;
    else if constexpr (std::is_same_v<T, uint16_t>) return ImGuiDataType_U16;
    else if constexpr (std::is_same_v<T, int32_t>) return ImGuiDataType_S32;
    else if constexpr (std::is_same_v<T, uint32_t>) return ImGuiDataType_U32;
    else if constexpr (std::is_same_v<T, int64_t>) return ImGuiDataType_S64;
    else if constexpr (std::is_same_v<T, uint64_t>) return ImGuiDataType_U64;
    else if constexpr (std::is_same_v<T, float>) return ImGuiDataType_Float;
    else if constexpr (std::is_same_v<T, double>) return ImGuiDataType_Double;
    else return ImGuiDataType_COUNT;
}

template <class... Args>
void ImGui::Overlay(const ImVec2& padding, ImGuiOverlayCorner corner, const char* text, Args&&... args) {
    // Adapted from imgui_demo.cpp.

    // Window flags to make the overlay be fixed, immobile, and have no decoration
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav
                           | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;

    // Get main viewport
    ImGuiViewport& viewport = *GetMainViewport();

    // Use work area to avoid any menubars or taskbars
    ImVec2 workPos = viewport.WorkPos;
    ImVec2 workSize = viewport.WorkSize;

    using enum ImGuiOverlayCorner;
    bool isRight = (corner == TopRight) || (corner == BottomRight);
    bool isBottom = (corner == BottomLeft) || (corner == BottomRight);

    // Window position calculations
    ImVec2 windowPos{
        isRight ? (workPos.x + workSize.x - padding.x) : (workPos.x + padding.x), // Offset X
        isBottom ? (workPos.y + workSize.y - padding.y) : (workPos.y + padding.y) // Offset Y
    };

    ImVec2 windowPosPivot{ static_cast<float>(isRight), static_cast<float>(isBottom) };

    // Window configuration
    SetNextWindowBgAlpha(0.5f);
    SetNextWindowPos(windowPos, ImGuiCond_Always, windowPosPivot);
    SetNextWindowViewport(viewport.ID);

    // Draw the window - we're passing the text as the window name (which doesn't show). This function will work as
    // long as every call has a different text value.
    if (Begin(text, nullptr, flags)) Text(text, std::forward<Args>(args)...);
    End();
}
