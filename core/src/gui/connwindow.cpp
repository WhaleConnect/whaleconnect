// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <imgui/imgui.h>

#include "connwindow.hpp"
#include "net/sockets.hpp"
#include "sys/error.hpp"
#include "util/imguiext.hpp"
#include "util/formatcompat.hpp"

ConnWindow::ConnWindow(const Sockets::DeviceData& data, std::string_view title, std::string_view extraInfo)
    : _title(title) {
    // Set the window text (not in initializer list because of length)
    // If there's no extra info, use the title as the window text; otherwise, format both the extra info and title
    // into the window text.
    _windowText = (extraInfo.empty()) ? std::string{ title } : std::format("({}) {}", extraInfo, title);

    _connect(data);
}

Task<> ConnWindow::_connect(const Sockets::DeviceData& data) {
    _output.addInfo("Connecting...");

    auto socket = co_await Sockets::createClientSocket(data);

    if (socket) {
        _socket = std::move(*socket);
        _output.addInfo("Connected.");
        _connected = true;
    } else {
        _errorHandler(socket);
    }
}

Task<> ConnWindow::_sendHandler(std::string_view s) {
    auto sendRet = co_await Sockets::sendData(_socket.get(), s);
    if (!sendRet) _errorHandler(sendRet);
}

Task<> ConnWindow::_readHandler() {
    if (!_connected) co_return;

    auto recvRet = co_await Sockets::recvData(_socket.get());
    if (!recvRet) {
        _errorHandler(recvRet);
        co_return;
    }

    if (recvRet->bytesRead == 0) {
        // Peer closed connection (here, 0 does not necessarily mean success, i.e. NO_ERROR)
        _output.addInfo("Remote host closed connection.");
        _connected = false;
    } else {
        _output.addText(recvRet->data);
    }
}

void ConnWindow::update() {
    _readHandler();

    // Set window size
    ImGui::SetNextWindowSize({ 500, 300 }, ImGuiCond_FirstUseEver);

    // Begin window, draw console outputs
    if (ImGui::Begin(_windowText.c_str(), &_open)) _output.update();

    // End window
    ImGui::End();
}
