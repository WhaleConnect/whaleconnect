// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string> // std::string
#include <cstring> // std::strchr()
#include <imgui/imgui_internal.h>

enum ImGuiOverlayCorner_ {
    ImGuiOverlayCorner_TopLeft,
    ImGuiOverlayCorner_TopRight,
    ImGuiOverlayCorner_BottomLeft,
    ImGuiOverlayCorner_BottomRight
};
typedef int ImGuiOverlayCorner;

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
    void TextUnformatted(const std::string& s);

    /// <summary>
    /// An adapted InputScalar() function with operator handling removed. Only works with uint8_t/uint16_t types.
    /// </summary>
    /// <typeparam name="T">The type of the integer passed to the function (8- or 16-bit uint)</typeparam>
    /// <param name="label">The text to show next to the input</param>
    /// <param name="val">The variable to pass to the internal InputText()</param>
    /// <param name="min">The minimum value to allow (optional)</param>
    /// <param name="max">The maximum value to allow (optional)</param>
    template<class T>
    void UnsignedInputScalar(const char* label, T& val, unsigned long min = 0, unsigned long max = 0);

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
    void LoadingSpinner(const char* label);

    /// <summary>
    /// Create a tab item, but with no spacing around the top tab button.
    /// </summary>
    /// <param name="label">The text to display in the tab</param>
    /// <returns>If the tab is active and selected</returns>
    /// <remarks>
    /// Taken from https://github.com/ocornut/imgui/issues/4368#issuecomment-887209351.
    /// </remarks>
    bool BeginTabItemNoSpacing(const char* label);
}

template<class T>
void ImGui::UnsignedInputScalar(const char* label, T& val, unsigned long min, unsigned long max) {
    // Decide parameter datatype
    constexpr bool is8bit = std::is_same_v<uint8_t, T>;
    constexpr bool is16bit = std::is_same_v<uint16_t, T>;
    static_assert(is8bit || is16bit, "This function only supports uint8_t/uint16_t data types");

    // Set maximum
    if (max == 0) max = (is8bit) ? 255UL : 65535UL;

    // Char buffer to hold input
    constexpr int bufLen = 6;
    char buf[bufLen] = "";
    std::snprintf(buf, bufLen, "%d", val);

    BeginGroup();
    PushID(label);

    // Text filtering callback - only allow numeric digits (not including operators with InputCharsDecimal)
    auto filter = [](ImGuiInputTextCallbackData* data) -> int {
        return !(data->EventChar < 256 && std::strchr("0123456789", static_cast<char>(data->EventChar)));
    };

    // InputText widget
    SetNextItemWidth(50);
    if (InputText("", buf, bufLen, ImGuiInputTextFlags_CallbackCharFilter, filter)) {
        try {
            // Convert buffer to the appropriate datatype
            val = static_cast<T>(ImClamp(std::stoul(buf), min, max));
        } catch (std::exception&) {
            // Exception thrown during conversion, set variable to minimum
            val = static_cast<T>(min);
        }
    }

    // Style config
    const float spacing = 2; // Space between widgets
    const ImVec2 sz{ GetFrameHeight(), GetFrameHeight() }; // Button size

    // "-" button
    SameLine(0, spacing);
    if (ButtonEx("-", sz, ImGuiButtonFlags_Repeat)) if (val > min) val--;

    // "+" button
    SameLine(0, spacing);
    if (ButtonEx("+", sz, ImGuiButtonFlags_Repeat)) if (val < max) val++;

    // Label
    SameLine(0, spacing);
    TextUnformatted(label);

    PopID();
    EndGroup();
}

template<class... Args>
void ImGui::Overlay(ImVec2 padding, ImGuiOverlayCorner corner, const char* text, Args... args) {
    // Window flags to make the overlay be fixed, immobile, and have no decoration
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;

    // Get main viewport
    const ImGuiViewport* viewport = GetMainViewport();

    // Use work area to avoid any menubars or taskbars
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;
    bool isRight = (corner == ImGuiOverlayCorner_TopRight) || (corner == ImGuiOverlayCorner_BottomRight);
    bool isBottom = (corner == ImGuiOverlayCorner_BottomLeft) || (corner == ImGuiOverlayCorner_BottomRight);

    // Window position calculations
    ImVec2 windowPos{
        isRight ? (workPos.x + workSize.x - padding.x) : (workPos.x + padding.x), // Offset X
        isBottom ? (workPos.y + workSize.y - padding.y) : (workPos.y + padding.y) // Offset Y
    };

    ImVec2 windowPosPivot{ static_cast<float>(isRight), static_cast<float>(isBottom) };

    // Window configuration
    SetNextWindowBgAlpha(0.5f);
    SetNextWindowPos(windowPos, ImGuiCond_Always, windowPosPivot);
    SetNextWindowViewport(viewport->ID);

    // Draw the window - we're passing the text as the window name (which doesn't show). This function will work as
    // long as every call has a different text value.
    if (Begin(text, nullptr, flags)) Text(text, args...);
    End();
}
