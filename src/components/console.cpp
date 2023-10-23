// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <cmath>
#include <ctime>
#include <format>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include <imgui.h>
#include <unicode/smpdtfmt.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>

module components.console;
import gui.imguiext;
import utils.strings;

constexpr const char* selectAllShortcut = OS_MACOS ? "\uebb8A" : "Ctrl+A";
constexpr const char* copyShortcut = OS_MACOS ? "\uebb8C" : "Ctrl+C";

// Formatter used for creating timestamp strings
const auto formatter = [] {
    UErrorCode status = U_ZERO_ERROR; // Isolated from global scope
    return icu::SimpleDateFormat{ icu::UnicodeString{ "hh:mm:ss.SSS > " }, status };
}();

bool floatsEqual(float a, float b) {
    return std::abs(a - b) <= std::numeric_limits<float>::epsilon();
}

bool colorsEqual(const ImVec4& a, const ImVec4& b) {
    return floatsEqual(a.x, b.x) && floatsEqual(a.y, b.y) && floatsEqual(a.z, b.z) && floatsEqual(a.w, b.w);
}

icu::UnicodeString getTimestamp() {
    UDate current = icu::Calendar::getNow();
    icu::UnicodeString dateReturned;
    formatter.format(current, dateReturned);
    return dateReturned;
}

void Console::add(std::string_view s, const ImVec4& color, bool canUseHex, std::string_view hoverText) {
    // Avoid empty strings
    if (s.empty()) return;

    icu::UnicodeString newText = icu::UnicodeString::fromUTF8(s);
    auto hoverTextOpt = hoverText.empty() ? std::nullopt : std::optional<std::string>{ hoverText };

    // Determine if text goes on a new line
    if (items.empty() || items.back().text.endsWith('\n') || !colorsEqual(items.back().color, color))
        items.emplace_back(canUseHex, newText, "", color, getTimestamp(), hoverTextOpt);
    else items.back().text += newText;

    // Computing the string's hex representation here removes the need to do so every application frame.
    if (canUseHex)
        for (unsigned char c : s)
            items.back().textHex += icu::UnicodeString::fromUTF8(std::format("{:02X} ", static_cast<int>(c)));

    scrollToEnd = autoscroll; // Scroll to the end if autoscroll is enabled
}

void Console::drawContextMenu() {
    ImGui::BeginDisabled(!textSelect.hasSelection());
    if (ImGui::MenuItem("Copy", copyShortcut)) textSelect.copy();
    ImGui::EndDisabled();

    if (ImGui::MenuItem("Select all", selectAllShortcut)) textSelect.selectAll();
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

icu::UnicodeString Console::getLineAtIdx(size_t i) const {
    ConsoleItem item = items[i];
    const icu::UnicodeString& lineBase = showHex && item.canUseHex ? item.textHex : item.text;
    return showTimestamps ? item.timestamp + lineBase : lineBase;
}

void Console::update(std::string_view id) {
    using namespace ImGuiExt::Literals;

    ImVec2 size{ ImGuiExt::FILL, -ImGui::GetFrameHeightWithSpacing() };
    ImGui::BeginChild(id.data(), size, true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 1, std::round(0.05_fh) }); // Tighten line spacing

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

            std::string line;
            getLineAtIdx(i).toUTF8String(line);
            ImGuiExt::textUnformatted(line);

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

    ImGui::PopStyleVar();
    ImGui::EndChild();

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
