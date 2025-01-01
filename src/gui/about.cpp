// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "about.hpp"

#include <config.hpp>
#include <imgui.h>

#include "gui/imguiext.hpp"

void drawAboutWindow(bool& open) {
    if (!open) return;

    using namespace ImGuiExt::Literals;
    ImGui::SetNextWindowSize(25_fh * 20_fh, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("About", &open)) {
        ImGui::End();
        return;
    }

    static bool copy = false;
    if (copy) ImGui::LogToClipboard();

    ImGui::Text("WhaleConnect");
    ImGui::Text("Cross-platform network communication software");

    ImGui::SeparatorText("Version/Build");
    ImGui::Text("Version: %s", Config::version);
    ImGui::Text("Build: %s", Config::versionBuild);
    ImGui::Text("Git commit: %s", Config::gitCommitLong);

    ImGui::SeparatorText("System");
    ImGui::Text("Built for: %s, %s", Config::plat, Config::arch);

    if (copy) {
        ImGui::LogFinish();
        copy = false;
    }

    ImGui::Spacing();
    if (ImGui::Button("Copy")) copy = true;
    ImGui::End();
}

void drawLinksWindow(bool& open) {
    if (!open) return;

    using namespace ImGuiExt::Literals;
    ImGui::SetNextWindowSize(20_fh * 10_fh, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Links", &open)) {
        ImGui::End();
        return;
    }

    ImGui::TextWrapped("These are helpful links to get information and support.");
    ImGui::TextLinkOpenURL("Repository", "https://github.com/WhaleConnect/whaleconnect");
    ImGui::SameLine();
    ImGui::TextLinkOpenURL("Changelog", "https://github.com/WhaleConnect/whaleconnect/blob/main/docs/changelog.md");

    ImGui::End();
}
