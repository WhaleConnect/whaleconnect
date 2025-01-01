// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connwindow.hpp"

#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <botan/tls_exceptn.h>
#include <imgui.h>

#include "app/settings.hpp"
#include "gui/imguiext.hpp"
#include "gui/menu.hpp"
#include "net/device.hpp"
#include "net/enums.hpp"
#include "os/error.hpp"
#include "sockets/clientsocket.hpp"
#include "sockets/clientsockettls.hpp"
#include "sockets/delegates/delegates.hpp"

SocketPtr makeClientSocket(bool useTLS, ConnectionType type) {
    using enum ConnectionType;

    switch (type) {
        case TCP:
            if (useTLS) return std::make_unique<ClientSocketTLS>();
            [[fallthrough]];
        case UDP:
            return std::make_unique<ClientSocketIP>();
        case L2CAP:
        case RFCOMM:
            return std::make_unique<ClientSocketBT>();
        default:
            std::unreachable();
    }
}

ConnWindow::ConnWindow(std::string_view title, bool useTLS, const Device& device, std::string_view) :
    Window(title), socket(makeClientSocket(useTLS, device.type)) {
    if (Settings::GUI::systemMenu) Menu::addWindowMenuItem(getTitle());
    connect(device);
}

ConnWindow::~ConnWindow() {
    if (Settings::GUI::systemMenu) Menu::removeWindowMenuItem(getTitle());
    socket->cancelIO();
}

Task<> ConnWindow::connect(Device device) try {
    // Connect the socket
    console.addInfo("Connecting...");
    co_await socket->connect(device);

    console.addInfo("Connected.");
    connected = true;
} catch (const System::SystemError& error) {
    console.errorHandler(error);
} catch (const Botan::TLS::TLS_Exception& error) {
    console.addError(error.what());
}

Task<> ConnWindow::sendHandler(std::string s) try {
    co_await socket->send(s);
} catch (const System::SystemError& error) {
    console.errorHandler(error);
} catch (const Botan::TLS::TLS_Exception& error) {
    console.addError(error.what());
}

Task<> ConnWindow::readHandler() try {
    if (!connected || pendingRecv) co_return;
    pendingRecv = true;

    auto [complete, closed, data, alert] = co_await socket->recv(console.getRecvSize());

    if (complete) {
        if (closed) {
            // Peer closed connection
            console.addInfo("Remote host closed connection.");
            socket->close();
            connected = false;
        } else {
            console.addText(data);
        }
    }

    if (alert) {
        std::string desc = "ALERT";
        ImVec4 color{ 0, 0.6f, 0, 1 };
        if (alert->isFatal) {
            console.addMessage(std::format("FATAL: {}", alert->desc), desc, color);
            connected = false;
        } else {
            console.addMessage(alert->desc, desc, color);
        }
    }
    pendingRecv = false;
} catch (const System::SystemError& error) {
    console.errorHandler(error);
} catch (const Botan::TLS::TLS_Exception& error) {
    console.addError(error.what());
}

void ConnWindow::onBeforeUpdate() {
    using namespace ImGuiExt::Literals;

    ImGui::SetNextWindowSize(35_fh * 20_fh, ImGuiCond_Appearing);
    readHandler();
}

void ConnWindow::onUpdate() {
    if (auto sendString = console.updateWithTextbox()) sendHandler(*sendString);
}
