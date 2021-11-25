// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <imgui/imgui.h>

#include "connwindow.hpp"
#include "net/sockets.hpp"
#include "util/imguiext.hpp"
#include "util/formatcompat.hpp"

void ConnWindow::_sendHandler(const std::string& s) {
    if (_connected) {
        // Connected, send data and check for errors
        if (Sockets::sendData(_sockfd, s) == SOCKET_ERROR) _errorHandler();
    } else {
        // Not connected, output a message
        _output.addInfo("The socket is not connected.");
    }
}

void ConnWindow::_errorHandler(int err) {
    // Check for non-fatal errors with a non-blocking socket
    if (!Sockets::isFatal(err, true)) return;

    _closeConnection();
    _error = true;

    // Add error line to console
    _output.addError(Sockets::formatErr(err));
}

void ConnWindow::connectHandler() {
    // No need to check for this again
    _pollFlags &= ~POLLOUT;

    // Make sure the non-blocking connection REALLY succeeded
    // [`Sockets::getSocketErr()` calls `getsockopt(_sockfd, SOL_SOCKET, SO_ERROR, ...)` internally]
    // The error handling approach used in ConnWindow is taken from https://stackoverflow.com/a/17770524
    _errorHandler(Sockets::getSocketErr(_sockfd));

    // The `_error` flag (indicating something failed) could have been set at any point in the past (including the
    // above check).
    // The `_connected` flag indicates that this function has already been called, no need to go again.
    // If any of these flags are set we don't continue on in the function.
    if (_error || _connected) return;

    _output.addInfo("Connected."); // Add information message
    _connected = true; // Done connecting
    _pollFlags |= POLLIN; // Ready for data input
}

void ConnWindow::inputHandler() {
    // No input if the socket is not connected
    if (!_connected) return;

    // String to store received data
    std::string recvBuf;

    // Perform receive call and check status code
    int ret = Sockets::recvData(_sockfd, recvBuf);
    switch (ret) {
    case SOCKET_ERROR:
        // Error, print message
        _errorHandler();
        break;
    case 0:
        // Peer closed connection (here, 0 does not necessarily mean success, i.e. NO_ERROR)
        _output.addInfo("Remote host closed connection.");
        _closeConnection();
        break;
    default:
        // Receive successful, add the buffer to the Console to display
        _output.addText(recvBuf);
    }
}

void ConnWindow::update() {
    // Set window size
    ImGui::SetNextWindowSize({ 500, 300 }, ImGuiCond_FirstUseEver);

    // Begin window, draw console outputs
    if (ImGui::Begin(_windowText.c_str(), &_open)) _output.update();

    // End window
    ImGui::End();
}
