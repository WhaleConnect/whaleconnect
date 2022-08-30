// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief A class to manage multiple related @p Window objects
*/

#pragma once

#include <algorithm>
#include <concepts>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "window.hpp"

class WindowList {
    std::vector<std::unique_ptr<Window>> _windows; // All windows

    bool _validateDuplicate(std::string_view title) {
        return std::ranges::find_if(_windows, [title](const auto& current) {
            return current->getTitle() == title;
        }) == _windows.end();
    }

public:
    /**
     * @brief Adds a new window to the list.
     * @tparam T The type of the window to add
     * @tparam ...Args Arguments to construct the window object with
     * @param ...args Arguments to construct the window object with
     * @return If the window is unique and was added
    */
    template <class T, class... Args> requires std::derived_from<T, Window>
    bool add(Args&&... args) {
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        if (!_validateDuplicate(ptr->getTitle())) return false;

        ptr->init();
        _windows.push_back(std::move(ptr));
        return true;
    }

    /**
     * @brief Redraws all contained windows and deletes any that have been closed.
    */
    void update() {
        // Remove all closed windows
        std::erase_if(_windows, [](const auto& window) { return !window->isOpen(); });

        // Update all open windows
        for (const auto& i : _windows) i->update();
    }
};
