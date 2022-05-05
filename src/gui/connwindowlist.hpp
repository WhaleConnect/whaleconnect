// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief A custom container to store and manage multiple @p ConnWindow objects
*/

#pragma once

#include <string>
#include <memory> // std::unique_ptr
#include <string_view>

#include "connwindow.hpp"
#include "net/sockets.hpp"

/**
 * @brief A custom container to store and manage multiple @p ConnWindow objects.
 * @sa ConnWindow
*/
class ConnWindowList {
    std::vector<std::unique_ptr<ConnWindow>> _windows; // All connection windows

public:
    /**
     * @brief Adds a new window to the list.
     * @param data A @p DeviceData to initialize the window with
     * @param extraInfo A string prepended to the window title in parentheses (default is none)
     * @return If the window is unique and was added
    */
    bool add(const Sockets::DeviceData& data, std::string_view extraInfo = "");

    /**
     * @brief Redraws all contained windows and deletes any that have been closed.
    */
    void update();
};
