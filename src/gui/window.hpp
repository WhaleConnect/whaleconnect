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
    bool _open = true; // If the window is open
    bool* _openPtr = &_open; // The pointer passed to ImGui::Begin

    bool _initialized = false; // If the initialize function has been called

    // Performs initialization required by a window object.
    virtual void _init() { /* May optionally be overridden in derived classes */ }

    // Always runs on every frame, before _updateContents is called.
    virtual void _beforeUpdate() { /* May optionally be overridden in derived classes */ }

    // Redraws the contents of the window. Must be overridden in derived classes.
    virtual void _updateContents() = 0;

protected:
    // Enables or disables the window's close button.
    void _setClosable(bool closable) { _openPtr = closable ? &_open : nullptr; }

public:
    /**
     * @brief Sets the window title.
     * @param title The title
    */
    Window(std::string_view title) : _title(title) {}

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
        _beforeUpdate();

        // Render window
        if (ImGui::Begin(_title.c_str(), _openPtr)) _updateContents();
        ImGui::End();
    }
};
