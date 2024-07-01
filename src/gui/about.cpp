// Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "about.hpp"

#include <config.hpp>
#include <imgui.h>

#include "gui/imguiext.hpp"

void drawLink(const char* label, const char* link) {
    ImGui::PushID(label);
    bool copy = ImGui::SmallButton("Copy");
    ImGui::PopID();

    ImGui::SameLine();
    ImGui::Text("%s:", label);
    ImGui::SameLine();

    if (copy) ImGui::LogToClipboard();
    ImGui::Text("https://%s", link);
    if (copy) ImGui::LogFinish();
}

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
    ImGui::SetNextWindowSize(40_fh * 10_fh, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Links", &open)) {
        ImGui::End();
        return;
    }

    drawLink("Repository", "github.com/WhaleConnect/whaleconnect");
    drawLink("Changelog", "github.com/WhaleConnect/whaleconnect/blob/main/docs/changelog.md");

    ImGui::End();
}
