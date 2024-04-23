// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "newconn.hpp"

#include <format>
#include <string>
#include <string_view>

#include "imguiext.hpp"
#include "newconnbt.hpp"
#include "newconnip.hpp"
#include "notifications.hpp"
#include "components/connwindow.hpp"
#include "net/device.hpp"
#include "net/enums.hpp"
#include "utils/strings.hpp"

// Formats a Device instance into a string for use in a ConnWindow title.
std::string formatDevice(bool useTLS, const Device& device, std::string_view extraInfo) {
    // Type of the connection
    bool isIP = device.type == ConnectionType::TCP || device.type == ConnectionType::UDP;
    const char* typeName = getConnectionTypeName(device.type);
    auto typeString = useTLS ? std::format("{}+TLS", typeName) : std::string{ typeName };

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

void addConnWindow(WindowList& list, bool useTLS, const Device& device, std::string_view extraInfo) {
    bool isNew = list.add<ConnWindow>(formatDevice(useTLS, device, extraInfo), useTLS, device, extraInfo);

    // If the connection exists, show a message
    if (!isNew) ImGuiExt::addNotification("This connection is already open.", NotificationType::Warning);
}

void drawNewConnectionWindow(bool& open, WindowList& connections, WindowList& sdpWindows) {
    if (!open) return;

    using namespace ImGuiExt::Literals;
    ImGui::SetNextWindowSize(40_fh * 11_fh, ImGuiCond_Appearing);

    if (ImGui::Begin("New Connection", &open) && ImGui::BeginTabBar("ConnectionTypes")) {
        drawIPConnectionTab(connections);
        drawBTConnectionTab(connections, sdpWindows);
        ImGui::EndTabBar();
    }
    ImGui::End();
}
