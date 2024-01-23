// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;

#include <imgui.h>

module gui.about;
import app.config;
import gui.imguiext;

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

    ImGui::Text("Network Socket Terminal");
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
