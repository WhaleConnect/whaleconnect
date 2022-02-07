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
    _output.addInfo("Connecting...");

    // Set the window text (not in initializer list because of length)
    // If there's no extra info, use the title as the window text; otherwise, format both the extra info and title
    // into the window text.
    _windowText = (extraInfo.empty()) ? std::string{ title } : std::format("({}) {}", extraInfo, title);

    _socket = Sockets::createClientSocket(data, _asyncData);

    if (!_socket) _errorHandler(); // Handle the error
}

void ConnWindow::_sendHandler(std::string_view s) {
    if (Sockets::sendData(_socket.get(), s) == SOCKET_ERROR) _errorHandler();
}

void ConnWindow::_errorHandler() {
    int err = System::getLastErr();

    // Check for non-fatal errors
    if (!System::isFatal(err)) return;

    // Add error line to console
    _output.addError(System::formatErr(err));
}

void ConnWindow::_connectHandler() {
    _output.addInfo("Connected.");
}

void ConnWindow::_disconnectHandler() {
    _output.addInfo("Disconnected.");
}

void ConnWindow::_readHandler() {
    //// String to store received data
    //std::string recvBuf;

    //// Perform receive call and check status code
    //int ret = Sockets::recvData(_socket.get(), recvBuf);
    //switch (ret) {
    //case SOCKET_ERROR:
    //    // Error, print message
    //    _errorHandler();
    //    break;
    //case 0:
    //    // Peer closed connection (here, 0 does not necessarily mean success, i.e. NO_ERROR)
    //    _output.addInfo("Remote host closed connection.");
    //    _closeConnection();
    //    break;
    //default:
    //    // Receive successful, add the buffer to the Console to display
    //    _output.addText(recvBuf);
    //}
    _output.addInfo("E");
}

void ConnWindow::update() {
    // Set window size
    ImGui::SetNextWindowSize({ 500, 300 }, ImGuiCond_FirstUseEver);

    // Begin window, draw console outputs
    if (ImGui::Begin(_windowText.c_str(), &_open)) _output.update();

    // End window
    ImGui::End();
}
