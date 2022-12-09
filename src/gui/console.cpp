// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "console.hpp"

#include <chrono>
#include <ctime>
#include <format>
#include <iomanip> // std::hex, std::uppercase, std::setw(), std::setfill()
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

void Console::_add(std::string_view s, const ImVec4& color, bool canUseHex) {
    // Avoid empty strings
    if (s.empty()) return;

    // Text goes on its own line if there are no items or the last line ends with a newline
    if (_items.empty() || (_items.back().text.back() == '\n'))
        _items.push_back({ canUseHex, std::string{ s }, {}, color, getTimestamp() });
    else _items.back().text += s;

    // Computing the string's hex representation here removes the need to recompute it every application frame.
    if (canUseHex) {
        // Build the hex representation of the string
        // We're appending to the last item's ostringstream (_items.back() is guaranteed to exist at this point).
        // We won't need to check for newlines using this approach.
        std::ostringstream& oss = _items.back().textHex;
        for (unsigned char c : s) {
            // Add each character:
            oss << std::hex            // Express integers in hexadecimal (i.e. base-16)
                << std::uppercase      // Make hex digits A-F uppercase
                << std::setw(2)        // Format in octets (8 bits => 2 hex digits)
                << std::setfill('0')   // If something is less than 2 digits it's padded with 0s (i.e. "A" => "0A")
                << static_cast<int>(c) // Use the above formatting rules to append the character's codepoint
                << " ";                // Separate octets with a single space
        }
    }

    _scrollToEnd = _autoscroll; // Scroll to the end if autoscroll is enabled
}

void Console::_updateOutput() {
    // Reserve space at bottom for more elements
    ImGui::BeginChild("output", { 0, -ImGui::GetFrameHeightWithSpacing() }, true, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4, 1 }); // Tighten line spacing

    // Add each item
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(_items.size()));
    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            const auto& item = _items[i];

            // Only color tuples with the last value set to 1 are considered
            bool hasColor = (item.color.w == 1.0f);

            // Timestamps
            if (_showTimestamps) {
                ImGui::TextUnformatted(item.timestamp);
                ImGui::SameLine();
            }

            // Apply color if needed
            if (hasColor) ImGui::PushStyleColor(ImGuiCol_Text, item.color);
            ImGui::TextUnformatted((_showHex && item.canUseHex) ? item.textHex.str() : item.text);
            if (hasColor) ImGui::PopStyleColor();

            // Right-click context menu for each line
            if (ImGui::BeginPopupContextItem(std::to_string(i).c_str())) {
                if (ImGui::MenuItem("Copy line")) ImGui::SetClipboardText(item.text.c_str());
                ImGui::EndPopup();
            }
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

    // "Clear output" button
    if (ImGui::Button("Clear output")) _items.clear();

    // "Options" button
    ImGui::SameLine();
    if (ImGui::Button("Options...")) ImGui::OpenPopup("options");

    // Popup for more options
    if (ImGui::BeginPopup("options")) {
        ImGui::MenuItem("Autoscroll", nullptr, &_autoscroll);
        ImGui::MenuItem("Show timestamps", nullptr, &_showTimestamps);
        ImGui::MenuItem("Show hexadecimal", nullptr, &_showHex);

        // Options for the input textbox
        ImGui::Separator();
        ImGui::MenuItem("Clear texbox on send", nullptr, &_clearTextboxOnSubmit);
        ImGui::MenuItem("Add final line ending", nullptr, &_addFinalLineEnding);
        ImGui::EndPopup();
    }

    // Line ending combobox
    // The code used to calculate where to put the combobox is derived from
    // https://github.com/ocornut/imgui/issues/4157#issuecomment-843197490
    float comboWidth = 150.0f;
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - comboWidth));
    ImGui::SetNextItemWidth(comboWidth);
    ImGui::Combo("##lineEnding", &_currentLE, "Newline\0Carriage return\0Both\0");
}

void Console::update() {
    ImGui::PushID("console");
    ImGui::BeginGroup();

    // Textbox
    float textboxHeight = 4.0f; // Number of lines that can be displayed
    ImVec2 size{ ImGui::FILL, ImGui::GetTextLineHeight() * textboxHeight };
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CtrlEnterForNewLine | ImGuiInputTextFlags_EnterReturnsTrue
                              | ImGuiInputTextFlags_AllowTabInput;

    // Apply foxus to textbox
    // An InputTextMultiline is an InputText contained within a child window so focus must be set before rendering it to
    // apply focus to the InputText.
    if (_focusOnTextbox) {
        ImGui::SetKeyboardFocusHere();
        _focusOnTextbox = false;
    }

    if (ImGui::InputTextMultiline("##input", _textBuf, size, flags)) {
        // Line ending
        const char* endings[] = { "\n", "\r", "\r\n" };
        auto selectedEnding = endings[_currentLE];

        // InputTextMultiline() always uses \n as a line ending, replace all occurences of \n with the selected ending
        std::string sendString = Strings::replaceAll(_textBuf, "\n", selectedEnding);

        // Add a final line ending if set
        if (_addFinalLineEnding) sendString += selectedEnding;

        // Invoke the callback function if the string is not empty
        if (!sendString.empty()) _inputCallback(sendString);

        // Blank out input textbox
        if (_clearTextboxOnSubmit) _textBuf.clear();

        _focusOnTextbox = true;
    }

    _updateOutput();

    ImGui::EndGroup();
    ImGui::PopID();
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
