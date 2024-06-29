// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "newserver.hpp"

#include <imgui.h>

#include "imguiext.hpp"
#include "notifications.hpp"
#include "components/serverwindow.hpp"
#include "net/device.hpp"
#include "net/enums.hpp"

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
    ImGuiExt::inputScalar("Port", serverInfo.port, 1, 10);

    ImGuiExt::radioButton("TCP", serverInfo.type, TCP);
    ImGuiExt::radioButton("UDP", serverInfo.type, UDP);
    ImGuiExt::radioButton("RFCOMM", serverInfo.type, RFCOMM);
    ImGuiExt::radioButton("L2CAP", serverInfo.type, L2CAP);

    // Cannot check the result of add since server titles are generated dynamically.
    if (ImGui::Button("Create Server")) servers.add<ServerWindow>("", serverInfo);

    ImGui::End();
}
