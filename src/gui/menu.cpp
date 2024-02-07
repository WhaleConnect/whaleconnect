// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module gui.menu;
import app.settings;
import components.windowlist;
import external.imgui;
import external.libsdl3;
import external.std;
import gui.imguiext;
import gui.notifications;

void windowMenu(WindowList& list, const char* desc) {
    if (!ImGui::BeginMenu(desc)) return;

    if (list.empty()) ImGui::TextDisabled("%s", std::format("No {}", desc).c_str());
    else
        for (const auto& i : list) ImGuiExt::windowMenuItem(i->getTitle());

    ImGui::EndMenu();
}

void Menu::drawMenuBar(bool& quit, WindowList& connections, WindowList& servers) {
    if (!ImGui::BeginMainMenuBar()) return;

    ImGuiExt::drawNotificationsMenu(notificationsOpen);

    if constexpr (OS_MACOS) {
        if (Settings::GUI::systemMenu) {
            ImGui::EndMainMenuBar();
            return;
        }
    }

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Settings", ImGuiExt::shortcut(',').data())) settingsOpen = true;
        ImGui::MenuItem("Quit", nullptr, &quit);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("New Connection", nullptr, nullptr))
            newConnectionOpen = true;
        if (ImGui::MenuItem("New Server", nullptr, nullptr))
            newServerOpen = true;
        if (ImGui::MenuItem("Notifications", nullptr, nullptr))
            notificationsOpen = true;
        ImGui::EndMenu();
    }

    // List all open connections and servers
    windowMenu(connections, "Connections");
    windowMenu(servers, "Servers");

    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("About", nullptr, nullptr)) aboutOpen = true;
        if (ImGui::MenuItem("Show Changelog"))
            SDL_OpenURL("https://github.com/NSTerminal/terminal/blob/main/docs/changelog.md");

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}
