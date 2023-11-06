// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <coroutine> // IWYU pragma: keep
#include <stdexcept>

#include <imgui.h>
#include <magic_enum.hpp>

module components.connwindow;
import gui.imguiext;
import net.device;
import net.enums;
import os.error;
import sockets.clientsocket;
import sockets.delegates.delegates;
import utils.strings;

SocketPtr makeSocket(ConnectionType type) {
    using enum ConnectionType;

    if (type == None) throw std::invalid_argument{ "Invalid socket type" };
    if (type == TCP || type == UDP) return std::make_unique<ClientSocket<SocketTag::IP>>();
    return std::make_unique<ClientSocket<SocketTag::BT>>();
}

ConnWindow::ConnWindow(std::string_view title, const Device& device, std::string_view) :
    Window(title), socket(makeSocket(device.type)) {
    connect(device);
}

Task<> ConnWindow::connect(Device device) try {
    // Connect the socket
    console.addInfo("Connecting...");
    co_await socket->connect(device);

    console.addInfo("Connected.");
    connected = true;
} catch (const System::SystemError& error) {
    console.errorHandler(error);
}

Task<> ConnWindow::sendHandler(std::string s) try {
    co_await socket->send(s);
} catch (const System::SystemError& error) {
    console.errorHandler(error);
}

Task<> ConnWindow::readHandler() try {
    if (!connected || pendingRecv) co_return;

    pendingRecv = true;
    auto recvRet = co_await socket->recv(console.getRecvSize());
    pendingRecv = false;

    if (recvRet) {
        console.addText(*recvRet);
    } else {
        // Peer closed connection
        console.addInfo("Remote host closed connection.");
        socket->close();
        connected = false;
    }
} catch (const System::SystemError& error) {
    console.errorHandler(error);
    pendingRecv = false;
}

void ConnWindow::onBeforeUpdate() {
    using namespace ImGuiExt::Literals;

    ImGui::SetNextWindowSize(35_fh * 20_fh, ImGuiCond_Appearing);
    readHandler();
}

void ConnWindow::onUpdate() {
    if (auto sendString = console.update()) sendHandler(*sendString);
}
