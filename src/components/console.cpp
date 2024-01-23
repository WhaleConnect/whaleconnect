// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <cmath>
#include <chrono>
#include <ctime>
#include <format>
#include <iterator>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include <imgui.h>
#include <utf8.h>

module components.console;
import gui.imguiext;
import utils.strings;

bool floatsEqual(float a, float b) {
    return std::abs(a - b) <= std::numeric_limits<float>::epsilon();
}

bool colorsEqual(const ImVec4& a, const ImVec4& b) {
    return floatsEqual(a.x, b.x) && floatsEqual(a.y, b.y) && floatsEqual(a.z, b.z) && floatsEqual(a.w, b.w);
}

std::string getTimestamp() {
    // Adapted from https://stackoverflow.com/a/35157784
    using namespace std::chrono;

    auto now = system_clock::now();

    // Get milliseconds (remainder from division into seconds)
    auto ms = (duration_cast<milliseconds>(now.time_since_epoch()) % 1s).count();

    // Get local time from current time point
    auto timer = system_clock::to_time_t(now);
    auto local = *std::localtime(&timer);

    // Return formatted string
    return std::format("{:02}:{:02}:{:02}.{:03}", local.tm_hour, local.tm_min, local.tm_sec, ms);
}

void Console::add(std::string_view s, const ImVec4& color, bool canUseHex, std::string_view hoverText) {
    // Avoid empty strings
    if (s.empty()) return;

    auto hoverTextOpt = hoverText.empty() ? std::nullopt : std::optional<std::string>{ hoverText };

    // Determine if text goes on a new line
    if (items.empty() || items.back().text.ends_with('\n') || !colorsEqual(items.back().color, color))
        items.emplace_back(canUseHex, "", "", color, getTimestamp(), hoverTextOpt);

    // Add text, fix invalid UTF-8 if necessary
    utf8::replace_invalid(s.begin(), s.end(), std::back_inserter(items.back().text));

    // Computing the string's hex representation here removes the need to do so every application frame.
    if (canUseHex)
        for (unsigned char c : s) items.back().textHex += std::format("{:02X} ", static_cast<int>(c));

    scrollToEnd = autoscroll; // Scroll to the end if autoscroll is enabled
}

void Console::drawTimestamps() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Link scrolling to main content
    ImGui::SetNextWindowScroll({ 0, yScrollPos });

    // The timestamps child window is shorter by ScrollbarSize to align with main content
    const float height = -ImGui::GetFrameHeightWithSpacing() - style.ScrollbarSize;

    // Calculate the width of the timestamps (always 12 chars) using the width of the "0" character
    ImVec2 size{ ImGui::CalcTextSize("0").x * 12, height };
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::BeginChild("timestamps", size, ImGuiChildFlags_AlwaysUseWindowPadding, flags);

    // Display visible timestamps
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(items.size()));
    while (clipper.Step())
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            ImGui::TextUnformatted(items[i].timestamp.c_str());

    clipper.End();

    ImGui::EndChild();
    ImGui::SameLine(0, 0);
}

void Console::drawContextMenu() {
    ImGui::BeginDisabled(!textSelect.hasSelection());
    if (ImGui::MenuItem("Copy", ImGuiExt::shortcut('C').data())) textSelect.copy();
    ImGui::EndDisabled();

    if (ImGui::MenuItem("Select all", ImGuiExt::shortcut('A').data())) textSelect.selectAll();
}

void Console::drawOptions() {
    // "Clear output" button
    if (ImGui::Button("Clear output")) clear();

    // "Options" button
    ImGui::SameLine();
    if (ImGui::Button("Options...")) ImGui::OpenPopup("options");

    // Popup for more options
    if (ImGui::BeginPopup("options")) {
        ImGui::MenuItem("Autoscroll", nullptr, &autoscroll);
        ImGui::MenuItem("Show timestamps", nullptr, &showTimestamps);
        ImGui::MenuItem("Show hexadecimal", nullptr, &showHex);

        ImGui::EndPopup();
    }
}

void Console::update(std::string_view id) {
    using namespace ImGuiExt::Literals;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 1, std::round(0.05_fh) }); // Tighten line spacing

    if (showTimestamps) drawTimestamps();

    ImVec2 size{ ImGuiExt::FILL, -ImGui::GetFrameHeightWithSpacing() };

    // Always show horizontal scrollbar to maintain a known content height / prevents occasional flickering on scroll
    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoMove;
    ImGui::BeginChild(id.data(), size, ImGuiChildFlags_Border, flags);

    // Add each item
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(items.size()));
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            const auto& item = items[i];

            // Only color tuples with the alpha value set are considered
            bool hasColor = item.color.w > 0.0f;

            // Apply color if needed
            if (hasColor) ImGui::PushStyleColor(ImGuiCol_Text, item.color);

            ImGuiExt::textUnformatted(getLineAtIdx(i));

            if (hasColor) ImGui::PopStyleColor();

            if (ImGui::IsItemHovered() && item.hoverText) {
                ImGui::BeginTooltip();
                ImGuiExt::textUnformatted((*item.hoverText).c_str());
                ImGui::EndTooltip();
            }
        }
    }
    clipper.End();

    // Scroll to end
    if (scrollToEnd) {
        ImGui::SetScrollHereX(1.0f);
        ImGui::SetScrollHereY(1.0f);
        scrollToEnd = false;
    }

    textSelect.update();

    if (ImGui::BeginPopupContextWindow()) {
        drawContextMenu();
        ImGui::EndPopup();
    }

    yScrollPos = ImGui::GetScrollY();
    ImGui::EndChild();
    ImGui::PopStyleVar();

    drawOptions();
}

void Console::addText(std::string_view s, std::string_view pre, const ImVec4& color, bool canUseHex,
    std::string_view hoverText) {
    // Split the string by newlines to get each line, then add each line
    for (auto start = s.begin(), end = start; end != s.end(); start = end) {
        end = std::find(start, s.end(), '\n'); // Find the next newline

        // Get substring
        if (end != s.end()) end++; // Increment end to include the newline in the substring
        add(std::string{ pre } + std::string{ start, end }, color, canUseHex, hoverText);
    }
}
