// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A custom container to store and manage multiple ConnWindow objects

#pragma once

#include <string>
#include <memory> // std::unique_ptr
#include <string_view>

#include "connwindow.hpp"
#include "net/sockets.hpp"

/// <summary>
/// A class to manage multiple ConnWindow objects.
/// </summary>
class ConnWindowList {
    using PtrType = std::unique_ptr<ConnWindow>; // A unique_ptr pointing to a managed ConnWindow object

    std::vector<PtrType> _windows; // All window pointers

public:
    /// <summary>
    /// Add a new, unique window to the list.
    /// </summary>
    /// <param name="data">The DeviceData to initialize the window with</param>
    /// <param name="extraInfo">A string prepended to the window title in parentheses (optional)</param>
    /// <returns>If the window is unique and added</returns>
    bool add(const Sockets::DeviceData& data, std::string_view extraInfo = "");

    /// <summary>
    /// Redraw all contained windows and delete any that have been closed.
    /// </summary>
    void update();
};
