// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief A class to represent a Dear ImGui window
*/

#pragma once

#include <string>
#include <string_view>

#include <imgui.h>

/**
 * @brief A class to represent a Dear ImGui window.
*/
class Window {
    std::string _title; // The window title
    ImVec2 _size; // The window size
    bool _open = true; // If the window is open

    bool _initialized = false; // If the initialize function has been called

    // Performs initialization required by a window object.
    virtual void _init() { /* May optionally be overridden in derived classes */ }

    // Redraws the contents of the window. Must be overridden in derived classes.
    virtual void _updateContents() = 0;

public:
    /**
     * @brief Sets the window information.
     * @param title The title
     * @param size The initial dimensions
    */
    Window(std::string_view title, const ImVec2& size = {}) : _title(title), _size(size) {}

    /**
     * @brief Virtual destructor provided for derived classes.
    */
    virtual ~Window() = default;

    /**
     * @brief Gets the window title.
     * @return The title
    */
    std::string_view getTitle() const { return _title; }

    /**
     * @brief Gets the window's open/closed state.
     * @return If the window is open
    */
    bool isOpen() const { return _open; }

    /**
     * @brief Performs any extra required initialization. This may be called once; subsequent calls will do nothing.
    */
    void init() {
        if (_initialized) return;

        _init();
        _initialized = true;
    }

    /**
     * @brief Updates the window and its contents.
    */
    void update() {
        // Set window size if provided
        if ((_size.x > 0) && (_size.y > 0)) ImGui::SetNextWindowSize(_size, ImGuiCond_FirstUseEver);

        // Render window
        if (ImGui::Begin(_title.c_str(), &_open)) _updateContents();
        ImGui::End();
    }
};
