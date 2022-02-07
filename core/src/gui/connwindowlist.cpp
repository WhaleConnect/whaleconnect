// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connwindowlist.hpp"
#include "util/formatcompat.hpp"

/// <summary>
/// Format a DeviceData instance into a string for use in a ConnWindow title.
/// </summary>
/// <param name="data">The DeviceData to format</param>
/// <returns>The resultant string</returns>
static std::string formatDeviceData(const Sockets::DeviceData& data) {
    // Type of the connection
    bool isBluetooth = Sockets::connectionTypeIsBT(data.type);

    // Bluetooth connections are described using the device's name (e.g. "MyESP32"),
    // IP-based connections use the device's IP address (e.g. 192.168.0.178).
    std::string deviceString = (isBluetooth) ? data.name : data.address;

    // Newlines may be present in a Bluetooth device name, and if they get into a window's title, anything after the
    // first one will get cut off (the title bar can only hold one line). Replace them with enter characters (U+23CE)
    // to keep everything on one line.
    deviceString = Strings::replaceAll(deviceString, "\n", "\u23CE");

    // Format the values into a string as the title
    // The address is always part of the id hash.
    // The port is not visible for a Bluetooth connection, instead, it is part of the id hash.
    const char* fmtStr = (isBluetooth) ? "{} Connection - {}##{} {}" : "{} Connection - {} port {}##{}";
    return std::format(fmtStr, Sockets::connectionTypeToStr(data.type), deviceString, data.port, data.address);
}

bool ConnWindowList::add(const Sockets::DeviceData& data, std::string_view extraInfo) {
    // Title of the ConnWindow
    std::string title = formatDeviceData(data);

    // Check if the title already exists
    bool isNew = std::find_if(_windows.begin(), _windows.end(), [title](const PtrType& current) {
        return current->getTitle() == title;
    }) == _windows.end();

    // Add the window to the list
    if (isNew) _windows.push_back(std::make_unique<ConnWindow>(data, title, extraInfo));

    return isNew;
}

void ConnWindowList::update() {
    // Iterate through the windows vector
    for (size_t i = 0; i < _windows.size(); i++) {
        // The current window
        auto& current = _windows[i];

        if (current->open()) {
            // Window is open, update it
            current->update();
        } else {
            // Window is closed, remove it from vector
            _windows.erase(_windows.begin() + i);

            // Iterators are invalid, wait until the next iteration
            break;
        }
    }
}
