// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string> // std::string
#include <cstring> // std::strchr()
#include <imgui/imgui_internal.h>

/// <summary>
/// Namespace containing add-on functions for Dear ImGui. Members of this namespace may not necessarily follow the style
/// guide in order to remain consistent with Dear ImGui's naming conventions. Unused return values or parameters present
/// in the original Dear ImGui API of which these functions are derived from may be removed to better fit the usage of
/// the functions.
/// </summary>
namespace ImGui {
    /// <summary>
    /// Wrapper function for ImGui::TextUnformatted() to allow a std::string parameter.
    /// </summary>
    /// <param name="s">The string to display</param>
    void TextUnformatted(const std::string& s);

    /// <summary>
    /// Wrapper function for ImGui::Text() to allow a uint16_t parameter.
    /// </summary>
    /// <param name="i">The uint16_t to display</param>
    void Text(uint16_t i);

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
    /// <param name="s">A pointer to the std::string buffer to use</param>
    /// <param name="flags">A set of ImGuiInputTextFlags to change how the textbox behaves</param>
    /// <returns>The value from InputText() called internally</returns>
    /// <remarks>
    /// The implementation for this function is copied from /misc/cpp/imgui_stdlib.cpp in the Dear ImGui GitHub repo,
    /// with slight modifications.
    /// </remarks>
    bool InputText(const char* label, std::string* s, ImGuiInputTextFlags flags = 0);

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
    /// <param name="location">An ImVec2 specifying the coordinates (from top-left corner) to put the overlay</param>
    /// <param name="text">The string to display in the overlay, accepts format specifiers</param>
    /// <param name="...args">Any parameters to format into the text</param>
    /// <remarks>
    /// Adapted from imgui_demo.cpp.
    /// </remarks>
    template<class... Args>
    void Overlay(ImVec2 location, const char* text, Args... args);
}

template<class T>
void ImGui::UnsignedInputScalar(const char* label, T& val, unsigned long min, unsigned long max) {
    // Decide data type of parameter
    ImGuiDataType dt;
    if (std::is_same_v<uint8_t, T>) dt = ImGuiDataType_U8; // Type of T is uint8_t
    else if (std::is_same_v<uint16_t, T>) dt = ImGuiDataType_U16; // Type of T is uint16_t
    else throw std::invalid_argument("This function only supports uint8_t/uint16_t data types");

    // Minimum and maximum
    if (max == 0) max = (dt == ImGuiDataType_U8) ? 255UL : 65535UL;

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
            // Convert buffer to unsigned long
            val = static_cast<T>(std::clamp(std::stoul(buf), min, max));
        } catch (std::exception&) {
            // Exception thrown during conversion, set variable to minimum
            val = static_cast<T>(min);
        }
    }

    // Style config
    const float widgetSpacing = 2;
    const ImVec2 sz = { GetFrameHeight(), GetFrameHeight() }; // Button size

    // Step buttons: Subtract
    SameLine(0, widgetSpacing);
    if (ButtonEx("-", sz, ImGuiButtonFlags_Repeat)) if (val > min) val--;

    // Step buttons: Add
    SameLine(0, widgetSpacing);
    if (ButtonEx("+", sz, ImGuiButtonFlags_Repeat)) if (val < max) val++;

    // Widget label
    SameLine(0, widgetSpacing);
    TextUnformatted(label);

    PopID();
    EndGroup();
}

template<class... Args>
void ImGui::Overlay(ImVec2 location, const char* text, Args... args) {
    // Window flags to make the overlay be fixed, immobile, and have no decoration
    int flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    // Get main viewport
    const ImGuiViewport* viewport = GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;

    // Window configuration
    SetNextWindowBgAlpha(0.5f);
    SetNextWindowPos({ workPos.x + location.x, workPos.y + location.y }, ImGuiCond_Always);
    SetNextWindowViewport(viewport->ID);

    // Draw the window - we're passing the text as the window name (which doesn't show). This function will work as long
    // as every call has a different text value.
    if (Begin(text, nullptr, flags)) Text(text, args...);
    End();
}
