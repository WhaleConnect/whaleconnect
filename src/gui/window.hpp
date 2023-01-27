// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>

#include <imgui.h>

// A class to represent a Dear ImGui window.
class Window {
    std::string _title;      // Window title
    bool _open = true;       // If this window is open
    bool* _openPtr = &_open; // Pointer passed to ImGui::Begin

    bool _initialized = false; // If the initialize function has been called

    // Performs initialization required by a window object.
    virtual void _init() { /* May be overridden optionally */ }

    // Always runs on every frame, before _onUpdate is called.
    virtual void _onBeforeUpdate() { /* May be overridden optionally */ }

    // Redraws the contents of the window. Must be overridden in derived classes.
    virtual void _onUpdate() = 0;

protected:
    // Enables or disables the window's close button.
    void _setClosable(bool closable) { _openPtr = closable ? &_open : nullptr; }

public:
    // Sets the window title.
    explicit Window(std::string_view title) : _title(title) {}

    // Virtual destructor provided for derived classes.
    virtual ~Window() = default;

    // Gets the window title.
    [[nodiscard]] std::string_view getTitle() const { return _title; }

    // Gets the window's open/closed state.
    [[nodiscard]] bool isOpen() const { return _open; }

    // Performs any extra required initialization. This may be called once; subsequent calls will do nothing.
    void init() {
        if (_initialized) return;

        _init();
        _initialized = true;
    }

    // Updates the window and its contents.
    void update() {
        _onBeforeUpdate();

        // Render window
        if (ImGui::Begin(_title.c_str(), _openPtr)) _onUpdate();
        ImGui::End();
    }
};
