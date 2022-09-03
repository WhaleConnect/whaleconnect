// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief Extension functions for Dear ImGui
 *
 * The naming conventions used in this file are selected to remain consistent with the official Dear ImGui API.
*/

#pragma once

#include <string>
#include <string_view>
#include <type_traits> // std::is_same_v

/**
 * @brief The corners of the application window where an overlay can be drawn.
 * @sa ImGui::Overlay()
*/
enum class ImGuiOverlayCorner { TopLeft, TopRight, BottomLeft, BottomRight };

namespace ImGui {
    constexpr float FILL = -FLT_MIN; /**< Make a widget fill a dimension. Use with @p ImVec2.*/

    /**
     * @brief A wrapper for @p TextUnformatted() to allow a @p string_view parameter.
     * @param s The string to display
    */
    inline void TextUnformatted(std::string_view s) {
        TextUnformatted(s.data());
    }

    /**
     * @brief A wrapper for @p RadioButton() to control a variable and its value.
     * @tparam T The type of the variable
     * @param label The widget label
     * @param var The variable to associate with the radiobutton
     * @param value The value to set to the variable when the radiobutton is selected
    */
    template <class T>
    void RadioButton(const char* label, T& var, T value) {
        if (RadioButton(label, var == value)) var = value;
    }

    /**
     * @brief Begins a child window with space at the bottom.
     * @param id The ID of the child window
     * @param space The space to reserve at the bottom (multiplied by frame height with item spacing)
     * @param border If the child window has a border
     * @param flags Flags to modify the child window
    */
    inline void BeginChildSpacing(const char* id, float space, bool border = false, ImGuiWindowFlags flags = 0) {
        BeginChild(id, { 0, space * -ImGui::GetFrameHeightWithSpacing() }, border, flags);
    }

    /**
     * @brief Gets the width of a rendered string added with the item inner spacing specified in the Dear ImGui style.
     * @param text The string to calculate the width from
     * @return The width of the string in pixels with the current font and item inner spacing settings
    */
    inline float CalcTextWidthWithSpacing(const char* text) {
        return GetStyle().ItemInnerSpacing.x + CalcTextSize(text).x;
    }

    /**
     * @brief Gets the @p ImGuiDataType of a given variable.
     * @tparam T A signed or unsigned numeric type
     * @param val The variable to get the type of
     * @return The corresponding @p ImGuiDataType enum for the given variable
    */
    template <class T>
    constexpr ImGuiDataType GetDataType(T val);

    /**
     * @brief A wrapper for @p InputScalar() with automatic type detection.
     * @tparam T The type of the numeric buffer
     * @tparam U The type of the step variables
     * @param label The widget label
     * @param data The numeric buffer to use
     * @param step Value change when the step buttons are clicked (0 to disable)
     * @param stepFast Value change when the step buttons are Ctrl-clicked (0 to disable)
    */
    template <class T, class U = int>
    inline void InputScalar(const char* label, T& data, U step = 0, U stepFast = 0) {
        // Any negative step value is considered invalid and nullptr is passed to disable the step buttons
        U* stepPtr = (step > 0) ? &step : nullptr;
        U* stepFastPtr = ((step > 0) && (stepFast > 0)) ? &stepFast : nullptr;

        InputScalar(label, GetDataType(data), &data, stepPtr, stepFastPtr);
    }

    /**
     * @brief A wrapper for @p InputText() to use a @p std::string buffer.
     * @param label The widget label
     * @param s The buffer to use
     * @param flags A set of @p ImGuiInputTextFlags to change how the textbox behaves
     * @return The value from @p InputText() called internally
    */
    bool InputText(const char* label, std::string& s, ImGuiInputTextFlags flags = 0);

    /**
     * @brief A wrapper for @p InputTextMultiline() to use a @p std::string buffer.
     * @param label The widget label
     * @param s The buffer to use
     * @param size The size of the textbox in pixels
     * @param flags A set of ImGuiInputTextFlags to change how the textbox behaves
     * @return The value from @p InputTextMultiline() called internally
    */
    bool InputTextMultiline(const char* label, std::string& s, const ImVec2& size = {}, ImGuiInputTextFlags flags = 0);

    /**
     * @brief Creates a (?) mark which shows a tooltip on hover.
     * @param desc The text to show in the tooltip
     *
     * Place this next to a widget to provide more details about it.
    */
    void HelpMarker(const char* desc);

    /**
     * @brief Creates a semi-transparent, fixed overlay on the application window.
     * @tparam ...Args A sequence of arguments to format into the text
     * @param padding The distance from the overlay to the specified corner
     * @param corner The corner to put the overlay in
     * @param text The string to display in the overlay, accepts format specifiers
     * @param ...args Parameters to format into the text
    */
    template<class... Args>
    void Overlay(const ImVec2& padding, ImGuiOverlayCorner corner, const char* text, Args&&... args);

    /**
     * @brief Displays a basic spinner which rotates every few frames.
     * @param label Text to display next to the spinner
    */
    inline void LoadingSpinner(const char* label) {
        // Taken from https://github.com/ocornut/imgui/issues/1901#issuecomment-400563921
        Text("%s... %c", label, "|/-\\"[static_cast<int>(GetTime() / 0.05f) & 3]);
    }
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
