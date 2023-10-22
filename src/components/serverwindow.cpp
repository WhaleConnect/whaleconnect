// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <array>
#include <coroutine> // IWYU pragma: keep
#include <format>
#include <map>

#include <imgui.h>
#include <imgui_internal.h>
#include <magic_enum.hpp>

module components.serverwindow;
import components.connwindow;
import gui.imguiext;
import net.enums;
import os.error;
import sockets.delegates.delegates;

const std::array colors{
    ImVec4{ 0.13f, 0.55f, 0.13f, 1 },
    ImVec4{ 0, 0.5f, 1, 1 },
    ImVec4{ 0.69f, 0.15f, 1, 1 },
    ImVec4{ 1, 0.27f, 0, 1 },
    ImVec4{ 1, 0.41f, 0.71f, 1 },
};

Task<> ServerWindow::Client::recv(IOConsole& serverConsole, std::string addr, unsigned int size) try {
    if (!connected || pendingRecv) co_return;

    pendingRecv = true;
    auto recvRet = co_await socket->recv(size);
    pendingRecv = false;

    if (recvRet) {
        serverConsole.addText(*recvRet, "", colors[colorIndex], true, addr);
        console.addText(*recvRet);
    } else {
        serverConsole.addInfo(std::format("{} closed connection.", addr));
        console.addInfo("Client closed connection.");
        socket->close();
        connected = selected = false;
    }
} catch (const System::SystemError& error) {
    serverConsole.errorHandler(error);
    pendingRecv = false;
}

Task<> ServerWindow::accept() try {
    if (!socket || pendingAccept) co_return;

    pendingAccept = true;

    auto [device, clientSocket] = co_await socket->accept();
    console.addInfo(std::format("Accepted connection from {} on port {}.", device.address, device.port));

    std::string key = std::format("{}|{}", device.address, device.port);
    clients.try_emplace(key, std::move(clientSocket), colorIndex);

    pendingAccept = false;
    colorIndex = (colorIndex + 1) % colors.size();
} catch (const System::SystemError& error) {
    console.errorHandler(error);
    pendingAccept = false;
}

void ServerWindow::a() {
    if (!ImGui::Begin("asdasd")) {
        ImGui::End();
        return;
    }

    ImGui::TextWrapped("Select clients to send data to");

    for (auto& [key, client] : clients) {
        ImGui::PushStyleColor(ImGuiCol_Text, colors[client.colorIndex]);
        ImGui::BeginDisabled(!client.connected);
        ImGui::Checkbox(key.c_str(), &client.selected);
        ImGui::EndDisabled();
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::PushID(key.c_str());
        if (ImGui::Button("\uecaf")) client.opened = true;

        ImGui::SameLine();
        if (ImGui::Button("\ueb99")) client.remove = true;
        ImGui::PopID();
    }

    ImGui::End();
}

void ServerWindow::onInit() {
    auto [port, ip] = socket->startServer({ ConnectionType::TCP, "", "127.0.0.1", 3000 });
    if (ip == IPType::None) console.addInfo(std::format("Server is active on port {}.", port));
    else console.addInfo(std::format("Server is active on port {} ({}).", port, magic_enum::enum_name(ip)));

    using namespace ImGuiExt::Literals;

    ImVec2 size{ 45_fh * 20_fh };

    ImGuiID id = ImGui::GetID("MainWindowGroup");
    ImGui::DockBuilderRemoveNode(id);
    ImGui::DockBuilderAddNode(id);
    ImGui::DockBuilderSetNodeSize(id, size);

    ImVec2 workCenter = ImGui::GetMainViewport()->GetWorkCenter();
    ImVec2 nodePos{ workCenter.x - size.x * 0.5f, workCenter.y - size.y * 0.5f };
    ImGui::DockBuilderSetNodePos(id, nodePos);

    ImGuiID dock1 = ImGui::DockBuilderSplitNode(id, ImGuiDir_Left, 0.7f, nullptr, &id);
    ImGuiID dock2 = ImGui::DockBuilderSplitNode(id, ImGuiDir_Right, 0.3f, nullptr, &id);

    ImGui::DockBuilderDockWindow("asdsa", dock1);
    ImGui::DockBuilderDockWindow("asdasd", dock2);
    ImGui::DockBuilderFinish(id);
}

void ServerWindow::onBeforeUpdate() {
    a();

    for (auto& [key, client] : clients) {
        using namespace ImGuiExt::Literals;
        ImGui::SetNextWindowSize(35_fh * 20_fh, ImGuiCond_Appearing);

        if (client.opened && ImGui::Begin(key.c_str(), &client.opened)) client.console.update("asd");
        ImGui::End();
    }

    accept();
}

void ServerWindow::onUpdate() {
    for (auto& client : clients)
        client.second.recv(console, client.first, console.getRecvSize());

    std::erase_if(clients, [](const auto& client) { return client.second.remove; });

    // Send data to all clients
    if (auto s = console.update())
        for (const auto& client : clients)
            if (client.second.selected && client.second.connected) client.second.socket->send(*s);
}
