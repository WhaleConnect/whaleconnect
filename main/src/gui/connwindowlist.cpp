// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connwindowlist.hpp"
#include "util/formatcompat.hpp"

std::string ConnWindowList::_formatDeviceData(const DeviceData& data) {
    // Type of the connection
    const char* type = Sockets::connectionTypesStr[data.type];
    bool isBluetooth = (data.type == Sockets::Bluetooth);

    // Bluetooth connections are described using the device's name (e.g. "MyESP32"),
    // IP connections (TCP/UDP) use the device's IP address (e.g. 192.168.0.178).
    std::string deviceString = (isBluetooth) ? data.name : data.address;

    // Newlines may be present in a Bluetooth device name, and if they get into a window's title, anything after the
    // first one will get cut off (the title bar can only hold one line). Replace them with spaces to keep everything on
    // one line.
    std::replace(deviceString.begin(), deviceString.end(), '\n', ' ');

    // Format the values into a string as the title
    // The address is always part of the id hash.
    // The port is not visible for a Bluetooth connection, instead, it is part of the id hash.
    const char* fmtStr = (isBluetooth) ? "{} Connection - {}##{} {}" : "{} Connection - {} port {}##{}";
    return std::format(fmtStr, type, deviceString, data.port, data.address);
}

void ConnWindowList::update() {
    for (auto& [title, window] : _windows) {
        if (*window) {
            // Window is open, update it
            window->update();
        } else {
            // Window is closed, remove it from vector
            _windows.erase(title);

            // After erasing, the iters are now invalid and we have to exit the loop and wait for the next iteration:
            break;
        }
    }
}
