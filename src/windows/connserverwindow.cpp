// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <coroutine>
#include <format>

#include <imgui.h>
#include <magic_enum.hpp>

module windows.connserverwindow;
import gui.imguiext;
import net.enums;
import os.error;
import sockets.delegates.delegates;
import windows.connwindow;

Task<> ConnServerWindow::accept() try {
    if (!socket || pendingAccept) co_return;

    pendingAccept = true;

    AcceptResult result = co_await socket->accept();
    console.addInfo(std::format("Accepted connection from {} on port {}", result.device.address, result.device.port));
    unopenedSockets.push_back(std::move(result));

    pendingAccept = false;
} catch (const System::SystemError& error) {
    console.errorHandler(error);
    pendingAccept = false;
}

void ConnServerWindow::onInit() {
    auto [port, ip] = socket->startServer({ ConnectionType::TCP, "", "127.0.0.1", 0 });
    if (ip == IPType::None) console.addInfo(std::format("Server is active on port {}", port));
    else console.addInfo(std::format("Server is active on port {} ({})", port, magic_enum::enum_name(ip)));
}

void ConnServerWindow::onBeforeUpdate() {
    connWindows.update();

    using namespace ImGui::Literals;

    ImGui::SetNextWindowSize(35_fh * 20_fh, ImGuiCond_Appearing);
    accept();
}

void ConnServerWindow::onUpdate() {
    // Send data to all clients
    if (auto s = console.update(1))
        for (const auto& window : connWindows)
            if (auto connWindow = dynamic_cast<ConnWindow*>(window.get())) connWindow->sendHandler(*s);

    ImGui::Text("%zu unopened clients", unopenedSockets.size());
    ImGui::SameLine();

    // Button to open connection windows to clients
    if (!unopenedSockets.empty() && ImGui::Button("Open all")) {
        for (auto& i : unopenedSockets)
            connWindows.add<ConnWindow>(std::move(i.socket), std::move(i.device), "Server");

        unopenedSockets.clear();
    }
}
