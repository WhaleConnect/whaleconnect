// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <coroutine>
#include <format>

#include <imgui.h>
#include <magic_enum.hpp>

module windows.connwindow;
import gui.imguiext;
import net.device;
import net.enums;
import os.error;
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
    std::string title
        = isIP ? std::format("{} Connection - {} port {}##{}", typeString, deviceString, device.port, device.address)
               : std::format("{} Connection - {}##{} port {}", typeString, deviceString, device.address, device.port);

    // If there's extra info, it is formatted before the window title.
    // If it were to be put after the title, it would be part of the invisible id hash (after the "##").
    return extraInfo.empty() ? title : std::format("({}) {}", extraInfo, title);
}

ConnWindow::ConnWindow(std::unique_ptr<Socket>&& socket, const Device& device, std::string_view extraInfo) :
    ConsoleWindow(formatDevice(device, extraInfo)), socket(std::move(socket)), device(device) {}

Task<> ConnWindow::connect() try {
    // Connect the socket
    addInfo("Connecting...");
    co_await socket->connect(device);

    addInfo("Connected.");
    connected = true;
} catch (const System::SystemError& error) {
    errorHandler(error);
}

Task<> ConnWindow::sendHandlerAsync(std::string s) try {
    co_await socket->send(s);
} catch (const System::SystemError& error) {
    errorHandler(error);
}

Task<> ConnWindow::readHandler(unsigned int size) try {
    if (!connected || pendingRecv) co_return;

    pendingRecv = true;
    auto recvRet = co_await socket->recv(size);
    pendingRecv = false;

    if (recvRet) {
        addText(*recvRet);
    } else {
        // Peer closed connection
        addInfo("Remote host closed connection.");
        socket->close();
        connected = false;
    }
} catch (const System::SystemError& error) {
    errorHandler(error);
    pendingRecv = false;
}

void ConnWindow::drawOptions() {
    using namespace ImGui::Literals;

    ImGui::Separator();
    ImGui::SetNextItemWidth(4_fh);
    ImGui::InputScalar("Receive size", recvSize);
}

void ConnWindow::onBeforeUpdate() {
    using namespace ImGui::Literals;

    ImGui::SetNextWindowSize(35_fh * 20_fh, ImGuiCond_Appearing);
    readHandler(recvSize);
}
