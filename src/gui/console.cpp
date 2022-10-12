// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <format>
#include <iomanip> // std::hex, std::uppercase, std::setw(), std::setfill()
#include <ranges> // std::views::split()

#if __cpp_lib_chrono >= 201803L
#include <chrono> // std::chrono
#endif

#include "console.hpp"
#include "util/strings.hpp"
#include "util/imguiext.hpp"

void Console::_add(std::string_view s, const ImVec4& color, bool canUseHex) {
    // Don't add an empty string
    // (highly unlikely, but still check as string::back() called on an empty string throws a fatal exception)
    if (s.empty()) return;

    // Get timestamp
#if __cpp_lib_chrono >= 201803L
    using namespace std::chrono;

    std::string timestamp = std::format("{:%T} >", current_zone()->to_local(system_clock::now()));
#else
    std::string timestamp;
#endif

    if (_items.empty() || (_items.back().text.back() == '\n')) {
        // Text goes on its own line if there are no items or the last line ends with a newline
        _items.push_back({ canUseHex, std::string{ s }, {}, color, timestamp });
    } else {
        // Text goes on the last line (append)
        _items.back().text += s;
    }

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
    ImGui::BeginChildSpacing("output", 1, true, ImGuiWindowFlags_HorizontalScrollbar);
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
        }
    }
    clipper.End();

    // Scroll to end if autoscroll is set
    if (_scrollToEnd) {
        ImGui::SetScrollHereX(1.0f);
        ImGui::SetScrollHereY(1.0f);
        _scrollToEnd = false;
    }

    // Clean up console
    ImGui::PopStyleVar();
    ImGui::EndChild();

    // "Clear output" button
    if (ImGui::Button("Clear output")) clear();

    // "Options" button
    ImGui::SameLine();
    if (ImGui::Button("Options...")) ImGui::OpenPopup("options");

    // Popup for more options
    if (ImGui::BeginPopup("options")) {
        ImGui::MenuItem("Autoscroll", nullptr, &_autoscroll);

#if __cpp_lib_chrono >= 201803L
        ImGui::MenuItem("Show timestamps", nullptr, &_showTimestamps);
#endif

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

void Console::update(float textboxHeight) {
    ImGui::PushID("Console");
    ImGui::BeginGroup();

    // Textbox
    if (ImGui::InputTextMultiline("##input", _textBuf, { ImGui::FILL, ImGui::GetTextLineHeight() * textboxHeight },
                                  ImGuiInputTextFlags_CtrlEnterForNewLine | ImGuiInputTextFlags_EnterReturnsTrue
                                  | ImGuiInputTextFlags_AllowTabInput)) {
        // Line ending
        std::array endings{ "\n", "\r", "\r\n" };
        auto selectedEnding = endings[_currentLE];

        // InputTextMultiline() always uses \n as a line ending, replace all occurences of \n with the selected ending
        std::string sendString = Strings::replaceAll(_textBuf, "\n", selectedEnding);

        // Add a final line ending if set
        if (_addFinalLineEnding) sendString += selectedEnding;

        // Invoke the callback function if the string is not empty
        if (!sendString.empty()) _inputCallback(sendString);

        if (_clearTextboxOnSubmit) _textBuf.clear(); // Blank out input textbox
        ImGui::SetItemDefaultFocus();
        ImGui::SetKeyboardFocusHere(-1); // Auto focus on input textbox
    }

    _updateOutput();

    ImGui::EndGroup();
    ImGui::PopID();
}

void Console::addText(std::string_view s, std::string_view pre, const ImVec4& color, bool canUseHex) {
    using namespace std::literals;

    // Split the string by newlines to get each line, then add each line
    for (auto i : std::views::split(s, "\n"sv)) {
        // Check if there's a newline after the current string:
        // Get the end position of the split string
        auto endIdx = std::ranges::distance(s.begin(), i.end());

        // Check if the end position is at the end of the input string
        // If it is, there will be nothing after so no newline is added.
        // Otherwise, there are more lines after, add a newline so subsequent lines are independent of this one.
        auto end = (s.begin() + endIdx == s.end()) ? "" : "\n";

        _add(std::format("{}{}{}", pre, std::string_view{ i }, end), color, canUseHex);
    }
}
