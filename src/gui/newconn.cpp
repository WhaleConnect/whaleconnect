// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <format>
#include <string>
#include <string_view>

#include <magic_enum.hpp>

module gui.newconn;
import components.connwindow;
import gui.notifications;
import net.device;
import net.enums;
import utils.strings;

// Formats a Device instance into a string for use in a ConnWindow title.
std::string formatDevice(const Device& device, std::string_view extraInfo) {
    // Type of the connection
    bool isIP = device.type == ConnectionType::TCP || device.type == ConnectionType::UDP;
    auto typeString = magic_enum::enum_name(device.type);

    // Bluetooth-based connections are described using the device's name (e.g. "MyESP32"),
    // IP-based connections use the device's IP address (e.g. 192.168.0.178).
    std::string deviceString = isIP ? device.address : device.name;

    // Newlines may be present in a Bluetooth device name, and if they get into a window's title, anything after the
    // first one will get cut off (the title bar can only hold one line). Replace them with left/down arrow icons
    // to keep everything on one line.
    deviceString = Strings::replaceAll(deviceString, "\n", "\uf306");

    // Format the values into a string as the title
    // The address is always part of the id hash.
    // The port is not visible for a Bluetooth connection, instead, it is part of the id hash.
    std::string title = isIP
        ? std::format("{} Connection - {} port {}##{}", typeString, deviceString, device.port, device.address)
        : std::format("{} Connection - {}##{} port {}", typeString, deviceString, device.address, device.port);

    // If there's extra info, it is formatted before the window title.
    // If it were to be put after the title, it would be part of the invisible id hash (after the "##").
    return extraInfo.empty() ? title : std::format("({}) {}", extraInfo, title);
}

void addConnWindow(WindowList& list, const Device& device, std::string_view extraInfo) {
    bool isNew = list.add<ConnWindow>(formatDevice(device, extraInfo), device, extraInfo);

    // If the connection exists, show a message
    if (!isNew) ImGuiExt::addNotification("This connection is already open.", NotificationType::Warning);
}
