// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>

#include <imgui.h>

// A class to represent a Dear ImGui window.
class Window {
    std::string title;     // Window title
    bool open = true;      // If this window is open
    bool* openPtr = &open; // Pointer passed to ImGui::Begin

    bool initialized = false; // If the initialize function has been called

    // Performs initialization required by a window object.
    virtual void onInit() {
        // May be overridden optionally
    }

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

    // Performs any extra required initialization. This may be called once; subsequent calls will do nothing.
    void init() {
        if (initialized) return;

        onInit();
        initialized = true;
    }

    // Updates the window and its contents.
    void update() {
        onBeforeUpdate();

        // Render window
        if (ImGui::Begin(title.c_str(), openPtr)) onUpdate();
        ImGui::End();
    }
};
