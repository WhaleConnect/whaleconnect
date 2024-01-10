// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <format>

#include <config.h>
#include <imgui.h>
#include <SDL3/SDL_misc.h>
#include <SDL3/SDL_platform.h>

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

void drawBuildInfo(bool& open) {
    if (!open) return;

    using namespace ImGuiExt::Literals;
    ImGui::SetNextWindowSize(25_fh * 20_fh, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("About", &open)) {
        ImGui::End();
        return;
    }

    static bool copy = false;
    if (copy) ImGui::LogToClipboard();

    ImGui::Text("Network Socket Terminal");
    ImGui::Text("Cross-platform network communication software");

    ImGui::SeparatorText("Version/Build");
    ImGui::Text("Version: " CONFIG_VERSION);
    ImGui::Text("Build: %ld", CONFIG_VERSION_BUILD);
    ImGui::Text("Git commit: " GIT_COMMIT_LONG);
    ImGui::Text("Dear ImGui: " IMGUI_VERSION " (%d)", IMGUI_VERSION_NUM);

    ImGui::SeparatorText("System");
    ImGui::Text("OS: %s", SDL_GetPlatform());

    if (copy) {
        ImGui::LogFinish();
        copy = false;
    }

    ImGui::Spacing();
    if (ImGui::Button("Copy")) copy = true;
    ImGui::End();
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

    static bool aboutOpen = false;

    if (ImGui::BeginMenu("Help")) {
        ImGui::MenuItem("About", nullptr, &aboutOpen);
        if (ImGui::MenuItem("Show Changelog"))
            SDL_OpenURL("https://github.com/NSTerminal/terminal/blob/main/docs/changelog.md");

        ImGui::Separator();
        if (ImGui::MenuItem("Open on GitHub")) SDL_OpenURL("https://github.com/NSTerminal/terminal");
        if (ImGui::MenuItem("View License")) SDL_OpenURL("https://github.com/NSTerminal/terminal/blob/main/COPYING");

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();

    drawBuildInfo(aboutOpen);
}
