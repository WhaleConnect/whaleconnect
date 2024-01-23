// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <format>
#include <string>
#include <string_view>

#include <imgui.h>
#include <nlohmann/json.hpp> // IWYU pragma: keep (fixes errors on MSVC)
#include <SDL3/SDL_misc.h>

#if OS_MACOS
#include <GUIMacOS-Swift.h>
#endif

module gui.menu;
import app.settings;
import components.windowlist;
import gui.imguiext;
import gui.notifications;

bool useSystemMenu = false;

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
        if (useSystemMenu) {
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

void Menu::setupMenuBar() {
    useSystemMenu = Settings::getSetting<bool>("gui.systemMenu");

#if OS_MACOS
    if (useSystemMenu) GUIMacOS::setupMenuBar();
#endif
}

void Menu::addWindowMenuItem([[maybe_unused]] std::string_view name) {
#if OS_MACOS
    GUIMacOS::addWindowMenuItem(std::string{ name });
#endif
}

void Menu::removeWindowMenuItem([[maybe_unused]] std::string_view name) {
#if OS_MACOS
    GUIMacOS::removeWindowMenuItem(std::string{ name });
#endif
}

void Menu::addServerMenuItem([[maybe_unused]] std::string_view name) {
#if OS_MACOS
    GUIMacOS::addServerMenuItem(std::string{ name });
#endif
}

void Menu::removeServerMenuItem([[maybe_unused]] std::string_view name) {
#if OS_MACOS
    GUIMacOS::removeServerMenuItem(std::string{ name });
#endif
}
