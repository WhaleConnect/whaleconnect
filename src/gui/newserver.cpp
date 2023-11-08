// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <string>

#include <imgui.h>

module gui.newserver;
import components.serverwindow;
import gui.imguiext;
import gui.notifications;
import net.device;
import net.enums;

void drawNewServerWindow(WindowList& servers, bool& open) {
    if (!open) return;

    using namespace ImGuiExt::Literals;

    ImGui::SetNextWindowSize(35_fh * 13_fh, ImGuiCond_Appearing);
    if (!ImGui::Begin("New Server", &open)) {
        ImGui::End();
        return;
    }

    using enum ConnectionType;
    static Device serverInfo{ TCP, "", "", 0 };

    if (serverInfo.type == TCP || serverInfo.type == UDP) {
        ImGui::SetNextItemWidth(15_fh);
        ImGuiExt::inputText("Address", serverInfo.address);

        ImGui::SameLine();
        if (ImGui::Button("IPv4")) serverInfo.address = "0.0.0.0";

        ImGui::SameLine();
        if (ImGui::Button("IPv6")) serverInfo.address = "::";

        ImGui::SameLine();
    }

    ImGui::SetNextItemWidth(7_fh);
    ImGuiExt::inputScalar("Port", serverInfo.port);

    ImGuiExt::radioButton("TCP", serverInfo.type, TCP);
    ImGuiExt::radioButton("UDP", serverInfo.type, UDP);
    ImGuiExt::radioButton("RFCOMM", serverInfo.type, RFCOMM);
    ImGuiExt::radioButton("L2CAP", serverInfo.type, L2CAP);

    if (ImGui::Button("Create Server") && !servers.add<ServerWindow>("", serverInfo))
        ImGuiExt::addNotification("This server already exists.", NotificationType::Warning);

    ImGui::End();
}
