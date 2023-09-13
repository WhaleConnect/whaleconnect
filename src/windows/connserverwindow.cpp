// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connserverwindow.hpp"

#include <format>

#include <imgui.h>
#include <magic_enum.hpp>

#include "connwindow.hpp"

#include "delegates/delegates.hpp"
#include "gui/imguiext.hpp"
#include "net/enums.hpp"
#include "os/error.hpp"

Task<> ConnServerWindow::_accept() try {
    if (!_socket || _pendingAccept) co_return;

    _pendingAccept = true;

    AcceptResult result = co_await _socket->accept();
    _addInfo(std::format("Accepted connection from {} on port {}", result.device.address, result.device.port));
    _unopenedSockets.push_back(std::move(result));

    _pendingAccept = false;
} catch (const System::SystemError& error) {
    _errorHandler(error);
    _pendingAccept = false;
}

void ConnServerWindow::_init() {
    auto [port, ip] = _socket->startServer({ ConnectionType::TCP, "", "127.0.0.1", 0 });
    if (ip == IPType::None) _addInfo(std::format("Server is active on port {}", port));
    else _addInfo(std::format("Server is active on port {} ({})", port, magic_enum::enum_name(ip)));
}

void ConnServerWindow::_onBeforeUpdate() {
    _connWindows.update();

    using namespace ImGui::Literals;

    ImGui::SetNextWindowSize(35_fh * 20_fh, ImGuiCond_Appearing);
    _accept();
}

void ConnServerWindow::_onUpdate() {
    _updateConsole(1);
    ImGui::Text("%zu unopened clients", _unopenedSockets.size());
    ImGui::SameLine();

    // Button to open connection windows to clients
    if (!_unopenedSockets.empty() && ImGui::Button("Open all")) {
        for (auto& i : _unopenedSockets)
            _connWindows.add<ConnWindow>(std::move(i.socket), std::move(i.device), "Server");

        _unopenedSockets.clear();
    }
}

void ConnServerWindow::_sendHandler(std::string_view s) {
    for (const auto& window : _connWindows)
        if (auto connWindow = dynamic_cast<ConnWindow*>(window.get())) connWindow->_sendHandler(s);
}
