// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <memory>
#include <string_view>
#include <vector>

#include "window.hpp"

// Manages and updates Window objects.
class WindowList {
    std::vector<std::unique_ptr<Window>> windows; // List of window pointers

    // Checks if the list contains a window with the specified title.
    bool validateDuplicate(std::string_view title) {
        auto windowIter
            = std::ranges::find_if(windows, [title](const auto& current) { return current->getTitle() == title; });

        return windowIter == windows.end();
    }

public:
    // Adds a new window to the list.
    template <class T, class... Args>
    requires std::derived_from<T, Window> && std::constructible_from<T, std::string_view, Args...>
    bool add(std::string_view title, Args&&... args) {
        if (!validateDuplicate(title)) return false;

        // Create the pointer and add it to the list
        windows.push_back(std::make_unique<T>(title, std::forward<Args>(args)...));
        return true;
    }

    // Redraws all contained windows and deletes any that have been closed.
    void update() {
        // Remove all closed windows
        std::erase_if(windows, [](const auto& window) { return !window->isOpen(); });

        // Update all open windows
        for (const auto& i : windows) i->update();
    }

    // Expose functions from internal vector

    auto begin() {
        return windows.begin();
    }

    auto end() {
        return windows.end();
    }

    bool empty() const {
        return windows.empty();
    }
};
