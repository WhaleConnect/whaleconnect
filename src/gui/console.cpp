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
#include <unicode/smpdtfmt.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>

#include "imguiext.hpp"

#include "unicode/unistr.h"
#include "utils/strings.hpp"

#if OS_MACOS
constexpr const char* selectAllShortcut = "Cmd+A";
constexpr const char* copyShortcut = "Cmd+C";
#else
constexpr const char* selectAllShortcut = "Ctrl+A";
constexpr const char* copyShortcut = "Ctrl+C";
#endif

// Formatter used for creating timestamp strings
static const auto formatter = [] {
    UErrorCode status = U_ZERO_ERROR; // Isolated from global scope
    return icu::SimpleDateFormat{ icu::UnicodeString{ "hh:mm:ss.SSS > " }, status };
}();

static icu::UnicodeString getTimestamp() {
    UDate current = icu::Calendar::getNow();
    icu::UnicodeString dateReturned;
    formatter.format(current, dateReturned);
    return dateReturned;
}

void Console::_add(std::string_view s, const ImVec4& color, bool canUseHex) {
    // Avoid empty strings
    if (s.empty()) return;

    icu::UnicodeString newText = icu::UnicodeString::fromUTF8(s);

    // Text goes on its own line if there are no items or the last line ends with a newline
    if (_items.empty() || _items.back().text.endsWith('\n'))
        _items.emplace_back(canUseHex, newText, "", color, getTimestamp());
    else _items.back().text += newText;

    // Computing the string's hex representation here removes the need to recompute it every application frame.
    if (canUseHex)
        for (unsigned char c : s)
            _items.back().textHex += icu::UnicodeString::fromUTF8(std::format("{:02X} ", static_cast<int>(c)));

    _scrollToEnd = _autoscroll; // Scroll to the end if autoscroll is enabled
}

void Console::_drawContextMenu() {
    ImGui::BeginDisabled(!_textSelect.hasSelection());
    if (ImGui::MenuItem("Copy", copyShortcut)) _textSelect.copy();
    ImGui::EndDisabled();

    if (ImGui::MenuItem("Select all", selectAllShortcut)) _textSelect.selectAll();
}

icu::UnicodeString Console::_getLineAtIdx(size_t i) const {
    ConsoleItem item = _items[i];
    const icu::UnicodeString& lineBase = (_showHex && item.canUseHex) ? item.textHex : item.text;
    return _showTimestamps ? item.timestamp + lineBase : lineBase;
}

void Console::drawOptions() {
    ImGui::MenuItem("Autoscroll", nullptr, &_autoscroll);
    ImGui::MenuItem("Show timestamps", nullptr, &_showTimestamps);
    ImGui::MenuItem("Show hexadecimal", nullptr, &_showHex);
}

void Console::update(std::string_view id, const ImVec2& size) {
    ImGui::BeginChild(id.data(), size, true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 1, tSize(0.05f) }); // Tighten line spacing

    // Add each item
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(_items.size()));
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            const auto& item = _items[i];

            // Only color tuples with the alpha value set are considered
            bool hasColor = item.color.w > 0.0f;

            // Apply color if needed
            if (hasColor) ImGui::PushStyleColor(ImGuiCol_Text, item.color);

            std::string line;
            _getLineAtIdx(i).toUTF8String(line);
            ImGui::TextUnformatted(line);

            if (hasColor) ImGui::PopStyleColor();
        }
    }
    clipper.End();

    // Scroll to end
    if (_scrollToEnd) {
        ImGui::SetScrollHereX(1.0f);
        ImGui::SetScrollHereY(1.0f);
        _scrollToEnd = false;
    }

    _textSelect.update();

    if (ImGui::BeginPopupContextWindow()) {
        _drawContextMenu();
        ImGui::EndPopup();
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
