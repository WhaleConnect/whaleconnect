// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <imgui.h>

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
    _windowText = extraInfo.empty() ? std::string{ title } : std::format("({}) {}", extraInfo, title);

    _connect(data);
}

Task<> ConnWindow::_connect(const Sockets::DeviceData& data) try {
    _output.addInfo("Connecting...");

    _socket = co_await Sockets::createClientSocket(data);
    _output.addInfo("Connected.");
    _connected = true;
} catch (const System::SystemError& error) {
    _errorHandler(error);
}

Task<> ConnWindow::_sendHandler(std::string_view s) try {
    co_await Sockets::sendData(_socket.get(), s);
} catch (const System::SystemError& error) {
    _errorHandler(error);
}

Task<> ConnWindow::_readHandler() try {
    if (!_connected) co_return;
    if (_pendingRecv) co_return;

    _pendingRecv = true;
    const auto& recvRet = co_await Sockets::recvData(_socket.get());

    if (recvRet.bytesRead == 0) {
        // Peer closed connection (here, 0 does not necessarily mean success, i.e. NO_ERROR)
        _output.addInfo("Remote host closed connection.");
        _socket.reset();
        _connected = false;
    } else {
        _output.addText(recvRet.data);
    }

    _pendingRecv = false;
} catch (const System::SystemError& error) {
    // Don't handle errors caused by socket closure (this means this object has been destructed)
    if (error.code != WSA_OPERATION_ABORTED) {
        std::lock_guard<std::mutex> outputLock{ _outputMutex };
        _errorHandler(error);
    }
}

void ConnWindow::update() {
    _readHandler();

    // Set window size
    ImGui::SetNextWindowSize({ 500, 300 }, ImGuiCond_FirstUseEver);

    // Begin window, draw console outputs
    if (ImGui::Begin(_windowText.c_str(), &_open)) {
        std::lock_guard<std::mutex> outputLock{ _outputMutex };
        _output.update();
    }

    // End window
    ImGui::End();
}
