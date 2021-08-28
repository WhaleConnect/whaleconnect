// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A custom container to store and manage multiple ConnWindow objects

#pragma once

#include <string>
#include <memory>
#include <functional>

#include "connwindow.hpp"
#include "net/sockets.hpp"

typedef std::unique_ptr<ConnWindow> ConnWindowPtr;

/// <summary>
/// A class to manage multiple ConnWindow objects.
/// </summary>
class ConnWindowList {
    std::function<SOCKET(DeviceData)> _connectFunction;
    std::vector<ConnWindowPtr> _windows; // All window pointers and their corresponding titles

    std::vector<pollfd> _pfds;

    void _populateFds();

public:
    template <class Fn>
    ConnWindowList(Fn&& fn) : _connectFunction(fn) {}

    bool add(const DeviceData& data);

    /// <summary>
    /// Redraw all contained windows and delete any that have been closed.
    /// </summary>
    void update();
};
