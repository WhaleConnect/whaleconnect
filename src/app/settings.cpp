// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <fstream>
#include <string>
#include <string_view>
#include <thread>

#include <imgui.h>
#include <imgui_internal.h>
#include <nlohmann/json.hpp>

module app.settings;
import gui.imguiext;

const nlohmann::json defaultSettings{
    { "font.size", 15 }, // Application font height in pixels
    { "font.file", "" }, // Font file
    { "font.ranges", { 0x0020, 0x00FF } }, // Glyph ranges to load from font
    { "gui.roundedCorners", false }, // If corners in the UI are rounded
    { "gui.windowTransparency", false }, // If application windows have a transparent effect
    { "os.numThreads", 0 }, // Number of worker threads (0 to auto-detect)
    { "os.queueEntries", 128 }, // Number of entries in io_uring instances
};

nlohmann::json loadedSettings;

void drawFontRangesSettings(std::vector<ImWchar>& fontRanges) {
    using namespace ImGuiExt::Literals;

    size_t rangesSize = fontRanges.size();
    auto deletePos = fontRanges.end();

    ImGui::Spacing();
    ImGui::Text("Glyph ranges to load from font");

    // Iterate through ranges (every 2 codepoints)
    for (size_t i = 0; i < rangesSize; i += 2) {
        ImGui::PushID(i);

        constexpr ImGuiDataType type = ImGuiExt::getDataType<ImWchar>();
        ImGui::SetNextItemWidth(8_fh);
        ImGui::InputScalarN("##range", type, fontRanges.data() + i, 2, nullptr, nullptr, "%04X");

        // Show delete button if more than one range (over 2 codepoints)
        if (rangesSize > 2) {
            ImGui::SameLine();
            if (ImGui::Button("\uf1af")) deletePos = fontRanges.begin() + i;
        }

        ImGui::PopID();
    }

    if (deletePos != fontRanges.end()) fontRanges.erase(deletePos, deletePos + 2);

    // Add button
    ImGui::SameLine();
    if (ImGui::Button("\uea13")) {
        fontRanges.push_back(0);
        fontRanges.push_back(0);
    }
    ImGui::Spacing();
}

const nlohmann::json& Internal::getValue(std::string_view key) {
    return loadedSettings[key];
}

const nlohmann::json& Internal::getDefaultValue(std::string_view key) {
    return defaultSettings[key];
}

void Settings::load(std::string_view filePath) {
    std::ifstream f{ filePath.data() };
    if (f.fail()) {
        loadedSettings = defaultSettings;
    } else {
        try {
            loadedSettings = nlohmann::json::parse(f);
        } catch (const nlohmann::json::parse_error&) {
            loadedSettings = defaultSettings;
        }
    }
}

void Settings::save(std::string_view filePath) {
    std::ofstream f{ filePath.data() };
    f << loadedSettings.dump(4) << "\n";
}

void Settings::drawSettingsWindow(bool& open) {
    if (ImGui::IsKeyChordPressed(ImGuiMod_Shortcut | ImGuiKey_Comma)) open = true;

    if (!open) return;

    using namespace ImGuiExt::Literals;
    ImGui::SetNextWindowSize(36_fh * 30_fh, ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Settings", &open)) {
        ImGui::End();
        return;
    }

    // ========================= Font settings =========================
    ImGui::SeparatorText("Font");

    static auto fontFile = getSetting<std::string>("font.file");
    ImGui::SetNextItemWidth(18_fh);
    ImGuiExt::inputText("File (empty to use default font)", fontFile);

    static auto fontRanges = getSetting<std::vector<ImWchar>>("font.ranges");
    drawFontRangesSettings(fontRanges);

    static auto fontSize = getSetting<uint8_t>("font.size");
    ImGui::SetNextItemWidth(4_fh);
    ImGuiExt::inputScalar("Size", fontSize);

    // ========================= GUI settings =========================
    ImGui::Dummy({ 0, 1_fh });
    ImGui::SeparatorText("GUI");

    static auto guiRoundedCorners = getSetting<bool>("gui.roundedCorners");
    ImGui::Checkbox("Rounded corners", &guiRoundedCorners);

    static auto guiWindowTransparency = getSetting<bool>("gui.windowTransparency");
    ImGui::Checkbox("Window transparency (make windows have a transparent effect)", &guiWindowTransparency);

    // ========================= OS settings =========================
    ImGui::Dummy({ 0, 1_fh });
    ImGui::SeparatorText("OS");

    static auto osNumThreads = getSetting<uint8_t>("os.numThreads");
    static const int numThreads = std::thread::hardware_concurrency();
    ImGui::SetNextItemWidth(4_fh);
    ImGuiExt::inputScalar("Number of worker threads", osNumThreads);
    ImGui::SameLine();
    ImGui::Text("(0 to use auto-detected number: %d)", numThreads);

    static auto osQueueEntries = getSetting<uint8_t>("os.queueEntries");
    ImGui::SetNextItemWidth(4_fh);
    ImGuiExt::inputScalar("io_uring queue entries (Linux only)", osQueueEntries);

    // ========================= Actions =========================
    ImGui::Dummy({ 0, 1_fh });
    if (ImGui::Button("Discard Changes")) open = false;

    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        loadedSettings["font.file"] = fontFile;
        loadedSettings["font.ranges"] = fontRanges;
        loadedSettings["font.size"] = fontSize;

        loadedSettings["gui.roundedCorners"] = guiRoundedCorners;
        loadedSettings["gui.windowTransparency"] = guiWindowTransparency;

        loadedSettings["os.numThreads"] = osNumThreads;
        loadedSettings["os.queueEntries"] = osQueueEntries;
    }

    ImGui::Text("Restart the application to apply settings.");
    ImGui::End();
}
