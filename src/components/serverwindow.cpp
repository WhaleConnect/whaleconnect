// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <array>
#include <coroutine> // IWYU pragma: keep
#include <format>
#include <map>
#include <utility>

#include <imgui.h>
#include <imgui_internal.h>

module components.serverwindow;
import components.connwindow;
import gui.imguiext;
import gui.menu;
import net.enums;
import os.error;
import sockets.delegates.delegates;
import sockets.serversocket;

// Colors to display each client in
const std::array colors{
    ImVec4{ 0.13f, 0.55f, 0.13f, 1 }, // Green
    ImVec4{ 0, 0.5f, 1, 1 }, // Blue
    ImVec4{ 0.69f, 0.15f, 1, 1 }, // Purple
    ImVec4{ 1, 0.27f, 0, 1 }, // Orange
    ImVec4{ 1, 0.41f, 0.71f, 1 } // Pink
};

SocketPtr makeServerSocket(ConnectionType type) {
    using enum ConnectionType;

    if (type == None) std::unreachable();
    if (type == TCP || type == UDP) return std::make_unique<ServerSocket<SocketTag::IP>>();
    return std::make_unique<ServerSocket<SocketTag::BT>>();
}

std::string formatDevice(const Device& device) {
    return std::format("{}|{}", device.name.empty() ? device.address : device.name, device.port);
}

Task<> ServerWindow::Client::recv(IOConsole& serverConsole, const Device& device, unsigned int size) try {
    if (!connected || pendingRecv) co_return;

    pendingRecv = true;
    auto recvResult = co_await socket->recv(size);

    if (recvResult.closed) {
        serverConsole.addInfo(std::format("{} closed connection.", formatDevice(device)));
        console.addInfo("Client closed connection.");
        socket->close();
        connected = selected = false;
    } else {
        serverConsole.addText(recvResult.data, "", colors[colorIndex], true, formatDevice(device));
        console.addText(recvResult.data);
    }
    pendingRecv = false;
} catch (const System::SystemError& error) {
    serverConsole.errorHandler(error);
    pendingRecv = false;
}

ServerWindow::ServerWindow(std::string_view title, const Device& serverInfo) :
    Window(title), socket(makeServerSocket(serverInfo.type)), isDgram(serverInfo.type == ConnectionType::UDP) {
    startServer(serverInfo);
    clientsWindowTitle = std::format("Clients: {}", getTitle());

    using namespace ImGuiExt::Literals;

    // Combined size of server and clients windows
    ImVec2 size{ 45_fh * 20_fh };

    // Build docking layout
    ImGuiID id = ImGui::GetID(getTitle().data());
    ImGui::DockBuilderRemoveNode(id);
    ImGui::DockBuilderAddNode(id);
    ImGui::DockBuilderSetNodeSize(id, size);

    ImVec2 workCenter = ImGui::GetMainViewport()->GetWorkCenter();
    ImVec2 nodePos{ workCenter.x - size.x * 0.5f, workCenter.y - size.y * 0.5f };
    ImGui::DockBuilderSetNodePos(id, nodePos);

    ImGuiID dock1 = ImGui::DockBuilderSplitNode(id, ImGuiDir_Left, 0.7f, nullptr, &id);
    ImGuiID dock2 = ImGui::DockBuilderSplitNode(id, ImGuiDir_Right, 0.3f, nullptr, &id);

    ImGui::DockBuilderDockWindow(getTitle().data(), dock1);
    ImGui::DockBuilderDockWindow(clientsWindowTitle.c_str(), dock2);
    ImGui::DockBuilderFinish(id);
}

ServerWindow::~ServerWindow() {
    Menu::removeServerMenuItem(getTitle());
    if (socket) socket->cancelIO();
}

void ServerWindow::startServer(const Device& serverInfo) try {
    auto [port, ip] = socket->startServer(serverInfo);
    const char* ipType = getIPTypeName(ip);
    const char* type = getConnectionTypeName(serverInfo.type);

    // Format title and status messages
    std::string title;
    if (ip == IPType::None) {
        title = std::format("{} Server - port {}##{}", type, port, serverInfo.address);
        console.addInfo(std::format("Server is active on port {}.", port));
    } else {
        title = std::format("{} ({}) Server - port {}##{}", type, ipType, port, serverInfo.address);
        console.addInfo(std::format("Server is active on port {} ({}).", port, ipType));
    }

    setTitle(title);
    Menu::addServerMenuItem(title);
} catch (const System::SystemError& error) {
    console.errorHandler(error);
}

Task<> ServerWindow::accept() try {
    if (!socket->isValid() || pendingIO) co_return;

    pendingIO = true;
    auto [device, clientSocket] = co_await socket->accept();

    std::string message = device.name.empty()
        ? std::format("Accepted connection from {} on port {}.", device.address, device.port)
        : std::format("Accepted connection from {} ({}) on port {}.", device.name, device.address, device.port);

    console.addInfo(message);

    auto [it, didEmplace] = clients.try_emplace(device, std::move(clientSocket), colorIndex);
    if (didEmplace) {
        nextColor();
    } else {
        it->second.socket = std::move(clientSocket);
        it->second.connected = it->second.selected = true;
    }
    pendingIO = false;
} catch (const System::SystemError& error) {
    console.errorHandler(error);
    pendingIO = false;
}

Task<> ServerWindow::recvDgram() try {
    if (!socket->isValid() || pendingIO) co_return;

    pendingIO = true;
    auto [device, data] = co_await socket->recvFrom(console.getRecvSize());

    auto [it, didEmplace] = clients.try_emplace(device, nullptr, colorIndex);
    if (didEmplace) nextColor(); // Advance colors if there is data received from a new client

    console.addText(data, "", colors[it->second.colorIndex], true, formatDevice(device));
    it->second.console.addText(data);

    pendingIO = false;
} catch (const System::SystemError& error) {
    console.errorHandler(error);
    pendingIO = false;
}

void ServerWindow::nextColor() {
    colorIndex = (colorIndex + 1) % colors.size();
}

void ServerWindow::drawClientsWindow() {
    if (!ImGui::Begin(clientsWindowTitle.c_str())) {
        ImGui::End();
        return;
    }

    ImGui::TextWrapped("Select clients to send data to");

    for (auto& [key, client] : clients) {
        std::string formatted = formatDevice(key);

        // Checkbox for sending
        ImGui::PushStyleColor(ImGuiCol_Text, colors[client.colorIndex]);
        ImGui::BeginDisabled(!client.connected);
        ImGui::Checkbox(formatted.c_str(), &client.selected);
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
        ImGui::SameLine();

        // Button to open received data
        ImGui::PushID(formatted.c_str());
        if (ImGui::Button("\uecaf")) client.opened = true;

        // Button to close client
        ImGui::SameLine();
        if (ImGui::Button("\ueb99")) client.remove = true;
        ImGui::PopID();
    }

    ImGui::End();
}

void ServerWindow::onBeforeUpdate() {
    // Redraw all active clients
    std::erase_if(clients, [](const auto& client) { return client.second.remove; });
    drawClientsWindow();

    // Perform I/O on clients
    if (isDgram) {
        recvDgram();
    } else {
        accept();
        for (auto& client : clients) client.second.recv(console, client.first, console.getRecvSize());
    }

    // Draw opened client windows
    for (auto& [key, client] : clients) {
        if (client.opened) {
            using namespace ImGuiExt::Literals;
            ImGui::SetNextWindowSize(35_fh * 20_fh, ImGuiCond_Appearing);

            std::string clientTitle = std::format("{}: {}", formatDevice(key).c_str(), getTitle().data());
            if (ImGui::Begin(clientTitle.c_str(), &client.opened)) client.console.update("output");
            ImGui::End();
        }
    }
}

void ServerWindow::onUpdate() {
    // Send data to all clients
    if (auto s = console.updateWithTextbox()) {
        for (const auto& [key, client] : clients) {
            if (client.selected) {
                if (isDgram) socket->sendTo(key, *s);
                else if (client.connected) client.socket->send(*s);
            }
        }
    }
}
