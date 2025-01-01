// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>

#include <imgui.h>

// Represents a Dear ImGui window.
class Window {
    std::string title; // Window title
    bool open = true; // If this window is open
    bool* openPtr = &open; // Pointer passed to ImGui::Begin

    // Always runs on every frame, before onUpdate is called.
    virtual void onBeforeUpdate() {
        // May be overridden optionally
    }

    // Redraws the contents of the window. Must be overridden in derived classes.
    virtual void onUpdate() = 0;

protected:
    // Enables or disables the window's close button.
    void setClosable(bool closable) {
        openPtr = closable ? &open : nullptr;
    }

    void setTitle(std::string_view newTitle) {
        title = newTitle;
    }

public:
    // Sets the window title.
    explicit Window(std::string_view title) : title(title) {}

    // Virtual destructor provided for derived classes.
    virtual ~Window() = default;

    // Gets the window title.
    [[nodiscard]] std::string_view getTitle() const {
        return title;
    }

    // Gets the window's open/closed state.
    [[nodiscard]] bool isOpen() const {
        return open;
    }

    // Updates the window and its contents.
    void update() {
        onBeforeUpdate();

        // Render window
        if (ImGui::Begin(title.c_str(), openPtr)) onUpdate();
        ImGui::End();
    }
};
