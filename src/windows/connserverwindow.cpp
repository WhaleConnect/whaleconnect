// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connserverwindow.hpp"

#include <format>

#include <imgui.h>

#include "connwindow.hpp"

#include "gui/imguiext.hpp"
#include "os/error.hpp"

Task<> ConnServerWindow::_accept() try {
    if (!_socket || _pendingAccept) co_return;

    _pendingAccept = true;

    auto [device, socketPtr] = co_await _socket->accept();
    _addInfo(std::format("Accepted connection from {}", device.address));
    _connWindows.add<ConnWindow>(std::move(socketPtr), device, "asdsa");
} catch (const System::SystemError& error) {
    _errorHandler(error);
}

void ConnServerWindow::_onBeforeUpdate() {
    _connWindows.update();

    using namespace ImGui::Literals;

    ImGui::SetNextWindowSize(35_fh * 20_fh, ImGuiCond_Appearing);
    _accept();
}

void ConnServerWindow::_sendHandler(std::string_view s) {
    for (const auto& window : _connWindows)
        if (auto connWindow = dynamic_cast<ConnWindow*>(window.get())) connWindow->_sendHandler(s);
}
