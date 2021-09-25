// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Extension functions for Dear ImGui

#pragma once

#include <string> // std::string
#include <cstring> // std::strchr()
#include <type_traits> // std::is_same_v

#include <imgui/imgui_internal.h>

enum ImGuiOverlayCorner_ {
    ImGuiOverlayCorner_TopLeft,
    ImGuiOverlayCorner_TopRight,
    ImGuiOverlayCorner_BottomLeft,
    ImGuiOverlayCorner_BottomRight
};
using ImGuiOverlayCorner = int;

/// <summary>
/// Namespace containing add-on functions for Dear ImGui.
/// </summary>
/// <remarks>
/// The naming conventions used here are selected to remain consistent with the official Dear ImGui API.
/// </remarks>
namespace ImGui {
    /// <summary>
    /// Wrapper function for ImGui::TextUnformatted() to allow a std::string parameter.
    /// </summary>
    /// <param name="s">The string to display</param>
    inline void TextUnformatted(const std::string& s) {
        TextUnformatted(s.c_str());
    }

    /// <summary>
    /// Get the width of a rendered string added with the item inner spacing specified in the Dear ImGui style.
    /// </summary>
    /// <param name="text">The string to calculate the width from</param>
    /// <returns>The width of the string with the current font and item inner spacing settings</returns>
    inline float CalcTextWidthWithSpacing(const char* text) {
        return GetStyle().ItemInnerSpacing.x + CalcTextSize(text).x;
    }

    template <class T>
    constexpr ImGuiDataType GetDataType(T val);

    /// <summary>
    /// An easy-to-use InputScalar() function with automatic type detection.
    /// </summary>
    /// <typeparam name="T">The type of the buffer</typeparam>
    /// <typeparam name="U">The type of the step variables</typeparam>
    /// <param name="label">The widget label</param>
    /// <param name="data">The integer buffer to use</param>
    /// <param name="step">Value change when the step buttons are clicked (0 to disable the buttons)</param>
    /// <param name="stepFast">Value change when the step buttons are Ctrl-clicked (0 to disable this option)</param>
    template <class T, class U = int>
    inline void InputScalar(const char* label, T& data, U step = 0, U stepFast = 0) {
        // Any negative step value is considered invalid and nullptr is passed to disable the step buttons
        U* stepPtr = (step > 0) ? &step : nullptr;
        U* stepFastPtr = (stepFast > 0) ? &stepFast : nullptr;

        InputScalar(label, GetDataType(data), &data, stepPtr, stepFastPtr);
    }

    /// <summary>
    /// An adapted InputText() to use a std::string passed by reference.
    /// </summary>
    /// <param name="label">The text to show next to the input</param>
    /// <param name="s">The std::string buffer to use</param>
    /// <param name="flags">A set of ImGuiInputTextFlags to change how the textbox behaves</param>
    /// <returns>The value from InputText() called internally</returns>
    /// <remarks>
    /// Adapted from imgui_stdlib.cpp.
    /// </remarks>
    bool InputText(const char* label, std::string& s, ImGuiInputTextFlags flags = 0);

    /// <summary>
    /// An adapter InputTextMultiline() to use a std::string passed by reference.
    /// </summary>
    /// <param name="label">The text to show next to the input</param>
    /// <param name="s">The std::string buffer to use</param>
    /// <param name="size">The size of the textbox</param>
    /// <param name="flags">A set of ImGuiInputTextFlags to change how the textbox behaves</param>
    /// <returns>The value from InputText() called internally</returns>
    /// <remarks>
    /// Adapted from imgui_stdlib.cpp.
    /// </remarks>
    bool InputTextMultiline(const char* label, std::string& s, const ImVec2& size = {}, ImGuiInputTextFlags flags = 0);

    /// <summary>
    /// Create a (?) mark which shows a tooltip on hover. Placed next to an element to provide more details about it.
    /// </summary>
    /// <param name="desc">The text in the tooltip</param>
    /// <remarks>
    /// Adapted from imgui_demo.cpp.
    /// </remarks>
    void HelpMarker(const char* desc);

    /// <summary>
    /// Create a semi-transparent, fixed overlay on the application window.
    /// </summary>
    /// <typeparam name="...Args">Any parameters to format into the text</typeparam>
    /// <param name="padding">The distance from the overlay to the specified corner</param>
    /// <param name="corner">The corner to put the overlay in (use ImGuiOverlayCorner_)</param>
    /// <param name="text">The string to display in the overlay, accepts format specifiers</param>
    /// <param name="...args">Any parameters to format into the text</param>
    /// <remarks>
    /// Adapted from imgui_demo.cpp.
    /// </remarks>
    template<class... Args>
    void Overlay(ImVec2 padding, ImGuiOverlayCorner corner, const char* text, Args... args);

    /// <summary>
    /// Display a basic spinner which rotates every few frames.
    /// </summary>
    /// <param name="label">The text to display next to the spinner</param>
    /// <remarks>
    /// Taken from https://github.com/ocornut/imgui/issues/1901#issuecomment-400563921.
    /// </remarks>
    inline void LoadingSpinner(const char* label) {
        Text("%s... %c", label, "|/-\\"[static_cast<int>(GetTime() / 0.05f) & 3]);
    }
}

template <class T>
constexpr ImGuiDataType ImGui::GetDataType([[maybe_unused]] T val) {
    if constexpr (std::is_same_v<T, int8_t>) {
        return ImGuiDataType_S8;
    } else if constexpr (std::is_same_v<T, uint8_t>) {
        return ImGuiDataType_U8;
    } else if constexpr (std::is_same_v<T, int16_t>) {
        return ImGuiDataType_S16;
    } else if constexpr (std::is_same_v<T, uint16_t>) {
        return ImGuiDataType_U16;
    } else if constexpr (std::is_same_v<T, int32_t>) {
        return ImGuiDataType_S32;
    } else if constexpr (std::is_same_v<T, uint32_t>) {
        return ImGuiDataType_U32;
    } else if constexpr (std::is_same_v<T, int64_t>) {
        return ImGuiDataType_S64;
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        return ImGuiDataType_U64;
    } else if constexpr (std::is_same_v<T, float>) {
        return ImGuiDataType_Float;
    } else if constexpr (std::is_same_v<T, double>) {
        return ImGuiDataType_Double;
    } else {
        return ImGuiDataType_COUNT;
    }
}

template <class... Args>
void ImGui::Overlay(ImVec2 padding, ImGuiOverlayCorner corner, const char* text, Args... args) {
    // Window flags to make the overlay be fixed, immobile, and have no decoration
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;

    // Get main viewport
    ImGuiViewport& viewport = *GetMainViewport();

    // Use work area to avoid any menubars or taskbars
    ImVec2 workPos = viewport.WorkPos;
    ImVec2 workSize = viewport.WorkSize;
    bool isRight = (corner == ImGuiOverlayCorner_TopRight) || (corner == ImGuiOverlayCorner_BottomRight);
    bool isBottom = (corner == ImGuiOverlayCorner_BottomLeft) || (corner == ImGuiOverlayCorner_BottomRight);

    // Window position calculations
    ImVec2 windowPos{
        (isRight) ? (workPos.x + workSize.x - padding.x) : (workPos.x + padding.x), // Offset X
        (isBottom) ? (workPos.y + workSize.y - padding.y) : (workPos.y + padding.y) // Offset Y
    };

    ImVec2 windowPosPivot{ static_cast<float>(isRight), static_cast<float>(isBottom) };

    // Window configuration
    SetNextWindowBgAlpha(0.5f);
    SetNextWindowPos(windowPos, ImGuiCond_Always, windowPosPivot);
    SetNextWindowViewport(viewport.ID);

    // Draw the window - we're passing the text as the window name (which doesn't show). This function will work as
    // long as every call has a different text value.
    if (Begin(text, nullptr, flags)) Text(text, args...);
    End();
}
