// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "menu.hpp"

#include <format>
#include <string>
#include <string_view>

#include <imgui.h>

#if OS_MACOS
#include <GUIMacOS-Swift.h>
#endif

#include "imguiext.hpp"
#include "menu.state.hpp"
#include "notifications.hpp"
#include "app/settings.hpp"
#include "components/windowlist.hpp"

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
        if (ImGui::MenuItem("New Connection", nullptr, nullptr)) newConnectionOpen = true;
        if (ImGui::MenuItem("New Server", nullptr, nullptr)) newServerOpen = true;
        if (ImGui::MenuItem("Notifications", nullptr, nullptr)) notificationsOpen = true;
        ImGui::EndMenu();
    }

    // List all open connections and servers
    windowMenu(connections, "Connections");
    windowMenu(servers, "Servers");

    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("About", nullptr, nullptr)) aboutOpen = true;
        if (ImGui::MenuItem("Links")) linksOpen = true;

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void Menu::setWindowFocus(const char* title) {
    ImGui::SetWindowFocus(title);
}

void Menu::setupMenuBar() {
#if OS_MACOS
    GUIMacOS::setupMenuBar();
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
