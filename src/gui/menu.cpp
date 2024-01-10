// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <format>

#include <imgui.h>

module gui.menu;
import components.windowlist;
import gui.imguiext;
import gui.notifications;

void windowMenu(WindowList& list, const char* desc) {
    if (!ImGui::BeginMenu(desc)) return;

    if (list.empty()) ImGui::TextDisabled("%s", std::format("No {}", desc).c_str());
    else
        for (const auto& i : list) ImGuiExt::windowMenuItem(i->getTitle());

    ImGui::EndMenu();
}

void drawMenuBar(bool& quit, bool& settingsOpen, bool& newConnOpen, bool& notificationsOpen, bool& newServerOpen,
    WindowList& connections, WindowList& servers) {
    if (!ImGui::BeginMainMenuBar()) return;

    ImGuiExt::drawNotificationsMenu(notificationsOpen);

    if (ImGui::BeginMenu("File")) {
        ImGui::MenuItem("Settings", nullptr, &settingsOpen);
        ImGui::MenuItem("Quit", nullptr, &quit);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("New Connection", nullptr, &newConnOpen);
        ImGui::MenuItem("New Server", nullptr, &newServerOpen);
        ImGui::MenuItem("Notifications", nullptr, &notificationsOpen);
        ImGui::EndMenu();
    }

    // List all open connections and servers
    windowMenu(connections, "Connections");
    windowMenu(servers, "Servers");

    ImGui::EndMainMenuBar();
}
