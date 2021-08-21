// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A custom container to store and manage multiple ConnWindow objects

#pragma once

#include <vector> // std::vector
#include <string> // std::string
#include <memory> // std::unique_ptr

#include "connwindow.hpp"
#include "net/sockets.hpp"

/// <summary>
/// A class to manage multiple ConnWindow objects.
/// </summary>
class ConnWindowList {
	std::vector<std::unique_ptr<ConnWindow>> _windows; // Vector of all windows

	/// <summary>
	/// Format a DeviceData into a string.
	/// </summary>
	/// <param name="data">The DeviceData to format</param>
	/// <returns>The resultant string</returns>
	/// <remarks>
	/// This function is used to get a usable title+id for a ConnWindow using the passed DeviceData's variable fields.
	/// </remarks>
	std::string _formatDeviceData(const DeviceData& data);

public:
	/// <summary>
	/// Add a new ConnWindow to the list.
	/// </summary>
	/// <typeparam name="Fn">The ConnWindow connector function</typeparam>
	/// <typeparam name="...Args">Additional arguments to pass to the connector function</typeparam>
	/// <param name="data">The DeviceData instance to get a formatted title from</param>
	/// <param name="fn">The ConnWindow connector function</param>
	/// <param name="...args">Additional arguments to pass to the connector function</param>
	/// <returns>If the ConnWindow was added (aborts and returns false if this ConnWindow already exists)</returns>
	template <class Fn, class... Args>
	bool add(const DeviceData& data, Fn&& fn, Args&&... args);

	/// <summary>
	/// Redraw all contained windows and delete any that have been closed.
	/// </summary>
	void update();
};

template <class Fn, class... Args>
bool ConnWindowList::add(const DeviceData& data, Fn&& fn, Args&&... args) {
	// Title of the ConnWindow
    std::string title = _formatDeviceData(data);

    // Iterate through all open windows, check if the title matches
    for (const auto& i : _windows) if (*i == title) return false;

    // If this point is reached it means that the window is unique, push it to the vector
    _windows.push_back(std::make_unique<ConnWindow>(title, fn, args...));
    return true;
}
