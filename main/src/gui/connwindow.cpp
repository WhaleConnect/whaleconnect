// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <imgui/imgui.h>

#include "connwindow.hpp"
#include "net/sockets.hpp"
#include "util/imguiext.hpp"
#include "util/formatcompat.hpp"

void ConnWindow::connectHandler() {
    if (_error || _connected) return;

    _output.addInfo("Connected.");
    _connected = true;
}

void ConnWindow::inputHandler() {
    if (!_connected) return;

    std::string recvBuf;

    // Check status code
    switch (Sockets::recvData(_sockfd, recvBuf)) {
    case SOCKET_ERROR:
        // Error, print message
        errorHandler();
        break;
    case 0:
        // Peer closed connection
        _output.addInfo("Remote host closed connection.");
        _closeConnection();
        break;
    default:
        // Receiving done successfully, add each line of the buffer into the vector
        size_t pos = 0;
        std::string sub = "";

        // Find each newline-delimited substring in the buffer to extract each individual line
        while ((pos = recvBuf.find('\n')) != std::string::npos) {
            pos++; // Increment position to include the newline in the substring
            sub = recvBuf.substr(0, pos); // Get the substring
            _output.addText(sub); // Add the substring to the output
            recvBuf.erase(0, pos); // Erase the substring from the buffer
        }

        // Add the last substring to the output (all other substrings have been erased from the buffer, the only one
        // left is the one after the last \n, or the whole string if there are no \n's present)
        _output.addText(recvBuf);
    }
}

void ConnWindow::errorHandler() {
    int err = Sockets::getLastErr();

    // Check for non-fatal errors with a non-blocking socket
    if (!Sockets::isFatal(err, true)) return;

    _closeConnection();
    _error = true;

    // Add error line to console
    Sockets::NamedError ne = Sockets::getErr(err);
    _output.addError(std::format("{} ({}): {}", ne.name, err, ne.desc));
}

void ConnWindow::update() {
    ImGui::SetNextWindowSize({ 500, 300 }, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(_title.c_str(), &_open)) {
        ImGui::End();
        return;
    }

    // Draw console output
    _output.update([&](const std::string& text) {
        if (_connected) {
            int ret = Sockets::sendData(_sockfd, text);
            if (ret == SOCKET_ERROR) errorHandler();
        } else {
            _output.addInfo("The socket is not connected.");
        }
    });

    // End window
    ImGui::End();
}
