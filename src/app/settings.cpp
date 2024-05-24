// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settings.hpp"

#include <array>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <imgui.h>

#include "appcore.hpp"
#include "fs.hpp"
#include "gui/imguiext.hpp"
#include "utils/settingsparser.hpp"
#include "utils/uuids.hpp"

const auto settingsFilePath = AppFS::getSettingsPath() / "settings.ini";
SettingsParser parser;

void drawFontRangesSettings(std::vector<ImWchar>& fontRanges) {
    using namespace ImGuiExt::Literals;

    std::size_t rangesSize = fontRanges.size();
    auto deletePos = fontRanges.end();

    ImGui::Spacing();
    ImGui::Text("Glyph ranges to load from font");
    ImGui::PushID("ranges");

    // Iterate through ranges (every 2 codepoints)
    for (std::size_t i = 0; i < rangesSize; i += 2) {
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
    ImGui::PopID();
}

void drawBluetoothUUIDsSettings(std::vector<std::pair<std::string, UUIDs::UUID128>>& uuids) {
    using namespace ImGuiExt::Literals;

    auto deletePos = uuids.end();

    ImGui::Spacing();
    ImGui::Text("Bluetooth UUIDs");

    ImGui::BeginChild("uuids", { ImGuiExt::fill, 0 }, ImGuiChildFlags_AutoResizeY);

    for (std::size_t i = 0; i < uuids.size(); i++) {
        auto& [name, uuid] = uuids[i];

        ImGui::PushID(i);
        ImGui::SetNextItemWidth(6_fh);
        ImGuiExt::inputText("##name", name);

        // Cumulative sizes of each UUID component
        static std::array sizes{ 0, 4, 6, 8, 10, 16 };

        for (std::size_t i = 0; i < 5; i++) {
            std::uint8_t* start = uuid.data() + sizes[i];
            std::size_t components = sizes[i + 1] - sizes[i];

            ImGui::SameLine();

            // Show separator between components
            if (i > 0) {
                ImGui::Text("-");
                ImGui::SameLine();
            }

            ImGui::SetNextItemWidth(components * 2 * ImGui::GetFontSize());
            ImGui::PushID(i);
            ImGui::InputScalarN("##components", ImGuiDataType_U8, start, components, nullptr, nullptr, "%02X");
            ImGui::PopID();
        }

        if (uuids.size() > 1) {
            ImGui::SameLine();
            if (ImGui::Button("\uf1af")) deletePos = uuids.begin() + i;
        }

        ImGui::PopID();
    }

    if (deletePos != uuids.end()) uuids.erase(deletePos);

    // Add button
    ImGui::SameLine();
    if (ImGui::Button("\uea13")) uuids.emplace_back("newUUID", UUIDs::createFromBase(0x00000000));

    ImGui::EndChild();
    ImGui::Spacing();
    ImGui::PopID();
}

void Settings::load() {
    parser.load(settingsFilePath);

    Font::file = parser.get<std::string>("font", "file");
    Font::ranges = parser.get<std::vector<ImWchar>>("font", "ranges", { 0x0020, 0x00FF });
    Font::size = parser.get<std::uint8_t>("font", "size", 15);

    GUI::roundedCorners = parser.get<bool>("gui", "roundedCorners");
    GUI::windowTransparency = parser.get<bool>("gui", "windowTransparency");
    GUI::systemMenu = parser.get<bool>("gui", "systemMenu", true);

    OS::numThreads = parser.get<std::uint8_t>("os", "numThreads");
    OS::queueEntries = parser.get<std::uint8_t>("os", "queueEntries", 128);
    OS::bluetoothUUIDs = parser.get<std::vector<std::pair<std::string, UUIDs::UUID128>>>("os", "bluetoothUUIDs",
        {
            { "L2CAP", UUIDs::createFromBase(0x0100) },
            { "RFCOMM", UUIDs::createFromBase(0x0003) },
        });
}

void Settings::save() {
    parser.write(settingsFilePath);
}

void Settings::drawSettingsWindow(bool& open) {
    // Open on CTRL-,
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

    ImGui::SetNextItemWidth(18_fh);
    ImGuiExt::inputText("File (empty to use default font)", Font::file);

    drawFontRangesSettings(Font::ranges);

    ImGui::SetNextItemWidth(4_fh);
    ImGuiExt::inputScalar("Size", Font::size);

    // ========================= GUI settings =========================
    ImGui::Dummy({ 0, 1_fh });
    ImGui::SeparatorText("GUI");

    ImGui::Checkbox("Rounded corners", &GUI::roundedCorners);
    ImGui::Checkbox("Window transparency (make windows have a transparent effect)", &GUI::windowTransparency);
    ImGui::Checkbox("Use system menu bars (macOS only)", &GUI::systemMenu);

    // ========================= OS settings =========================
    ImGui::Dummy({ 0, 1_fh });
    ImGui::SeparatorText("OS");

    static const int numThreads = std::thread::hardware_concurrency();
    ImGui::SetNextItemWidth(4_fh);
    ImGuiExt::inputScalar("Number of worker threads", OS::numThreads);
    ImGui::SameLine();
    ImGui::Text("(0 to use auto-detected number: %d)", numThreads);

    ImGui::SetNextItemWidth(4_fh);
    ImGuiExt::inputScalar("io_uring queue entries (Linux only)", OS::queueEntries);

    drawBluetoothUUIDsSettings(OS::bluetoothUUIDs);

    // ========================= Actions =========================
    ImGui::Dummy({ 0, 1_fh });
    if (ImGui::Button("Discard Changes")) open = false;

    ImGui::SameLine();
    if (ImGui::Button("Apply")) {
        parser.set("font", "file", Font::file);
        parser.set("font", "ranges", Font::ranges);
        parser.set("font", "size", Font::size);

        parser.set("gui", "roundedCorners", GUI::roundedCorners);
        parser.set("gui", "windowTransparency", GUI::windowTransparency);
        parser.set("gui", "systemMenu", GUI::systemMenu);

        parser.set("os", "numThreads", OS::numThreads);
        parser.set("os", "queueEntries", OS::queueEntries);
        parser.set("os", "bluetoothUUIDs", OS::bluetoothUUIDs);

        AppCore::configOnNextFrame();
    }

    ImGui::End();
}
