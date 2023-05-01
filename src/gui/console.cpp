// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "console.hpp"

#include <array>
#include <chrono>
#include <ctime>
#include <format>
#include <string>
#include <string_view>

#include <imgui.h>

#include "imguiext.hpp"

#include "utils/strings.hpp"

static std::string getTimestamp() {
    using namespace std::chrono;

    auto now = system_clock::now();

    // Get milliseconds (remainder from division into seconds)
    auto ms = (duration_cast<milliseconds>(now.time_since_epoch()) % 1s).count();

    // Get local time from current time point
    auto timer = system_clock::to_time_t(now);
    auto local = *std::localtime(&timer);

    // Return formatted string
    return std::format("{:02}:{:02}:{:02}.{:03} >", local.tm_hour, local.tm_min, local.tm_sec, ms);
}

// Activation function for the context menu for each line.
static void lineContextMenu(int id, std::string_view s) {
    if (ImGui::BeginPopupContextItem(std::to_string(id).c_str())) {
        if (ImGui::MenuItem("Copy line")) ImGui::SetClipboardText(s.data());
        ImGui::EndPopup();
    }
}

void Console::_add(std::string_view s, const ImVec4& color, bool canUseHex) {
    // Avoid empty strings
    if (s.empty()) return;

    // Text goes on its own line if there are no items or the last line ends with a newline
    if (_items.empty() || (_items.back().text.back() == '\n'))
        _items.emplace_back(canUseHex, std::string{ s }, "", color, getTimestamp());
    else _items.back().text += s;

    // Computing the string's hex representation here removes the need to recompute it every application frame.
    if (canUseHex) {
        // Build the hex representation of the string
        // We're appending to the last item's ostringstream (_items.back() is guaranteed to exist at this point).
        // We won't need to check for newlines using this approach.
        for (unsigned char c : s) _items.back().textHex += std::format("{:02X} ", static_cast<int>(c));
    }

    _scrollToEnd = _autoscroll; // Scroll to the end if autoscroll is enabled
}

void Console::drawOptions() {
    ImGui::MenuItem("Autoscroll", nullptr, &_autoscroll);
    ImGui::MenuItem("Show timestamps", nullptr, &_showTimestamps);
    ImGui::MenuItem("Show hexadecimal", nullptr, &_showHex);
}

void Console::update(std::string_view id, const ImVec2& size) {
    ImGui::BeginChild(id.data(), size, true, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4, 1 }); // Tighten line spacing

    // Add each item
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(_items.size()));
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            const auto& item = _items[i];

            // Only color tuples with the alpha value set are considered
            bool hasColor = item.color.w > 0.0f;

            // Timestamps
            if (_showTimestamps) {
                ImGui::TextUnformatted(item.timestamp);
                ImGui::SameLine();
            }

            // Apply color if needed
            if (hasColor) ImGui::PushStyleColor(ImGuiCol_Text, item.color);
            ImGui::TextUnformatted((_showHex && item.canUseHex) ? item.textHex : item.text);
            if (hasColor) ImGui::PopStyleColor();

            // Right-click context menu for each line
            lineContextMenu(i, item.text);
        }
    }
    clipper.End();

    // Scroll to end
    if (_scrollToEnd) {
        ImGui::SetScrollHereX(1.0f);
        ImGui::SetScrollHereY(1.0f);
        _scrollToEnd = false;
    }

    ImGui::PopStyleVar();
    ImGui::EndChild();
}

void Console::addText(std::string_view s, std::string_view pre, const ImVec4& color, bool canUseHex) {
    // Split the string by newlines to get each line, then add each line
    for (auto start = s.begin(), end = start; end != s.end(); start = end) {
        end = std::find(start, s.end(), '\n'); // Find the next newline

        // Get substring
        if (end != s.end()) end++; // Increment end to include the newline in the substring
        _add(std::string{ pre } + std::string{ start, end }, color, canUseHex);
    }
}
