// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iomanip> // std::hex, std::uppercase, std::setw(), std::setfill()

// TODO: Remove the #if check once all major compilers implement timezones for <chrono>
#if __cpp_lib_chrono >= 201803L
#include <chrono> // std::chrono
#endif

#include <imgui/imgui.h>

#include "console.hpp"
#include "app/settings.hpp"
#include "util/strings.hpp"
#include "util/imguiext.hpp"
#include "util/formatcompat.hpp"

void Console::_add(std::string_view s, const ImVec4& color, bool canUseHex) {
    // Don't add an empty string
    // (highly unlikely, but still check as string::back() called on an empty string throws a fatal exception)
    if (s.empty()) return;

    // Get timestamp
    // TODO: Remove the #if check once all major compilers implement timezones for <chrono>
#if __cpp_lib_chrono >= 201803L
    using namespace std::chrono;

    // TODO: Limitations with {fmt} - `.time_since_epoch()` is added. Remove when standard std::format() is used
    std::string timestamp = std::format("{:%T} >", current_zone()->to_local(system_clock::now()).time_since_epoch());
#else
    std::string timestamp;
#endif

    if (_items.empty() || (_items.back().text.back() == '\n')) {
        // Text goes on its own line if there are no items or the last line ends with a newline
        // We need to explicitly specify the ostringstream constructor so the compiler knows what type the argument is.
        _items.emplace_back(canUseHex, std::string{ s }, std::ostringstream{}, color, timestamp);
    } else {
        // Text goes on the last line (append)
        _items.back().text += s;
    }

    // The code block below is located here as a "caching" mechanism - the hex representation is only computed when the
    // item is added. It is simply retreived in the main loop.
    // If this was in `update()` (which would make the code slightly simpler), this would get unnecessarily re-computed
    // every frame - 60 times a second or more.
    if (canUseHex) {
        // Build the hex representation of the string
        // We're appending to the last item's ostringstream (`_items.back()` is guaranteed to exist at this point).
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
    static const float reservedSpace = -ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("output", { 0, reservedSpace }, true, ImGuiWindowFlags_HorizontalScrollbar);
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

        // TODO: Remove the #if check once all major compilers implement timezones for <chrono>
#if __cpp_lib_chrono >= 201803L
        ImGui::MenuItem("Show timestamps", nullptr, &_showTimestamps);
#endif

        ImGui::MenuItem("Show hexadecimal", nullptr, &_showHex);

        // Options for the input textbox
        if (_hasInput) {
            ImGui::Separator();

            ImGui::MenuItem("Clear texbox on send", nullptr, &_clearTextboxOnSend);

            ImGui::MenuItem("Add final line ending", nullptr, &_addFinalLineEnding);
        }

        ImGui::EndPopup();
    }

    // Line ending combobox, don't draw this if there's no input textbox
    if (_hasInput) {
        // The code used to calculate where to put the combobox is derived from
        // https://github.com/ocornut/imgui/issues/4157#issuecomment-843197490
        float comboWidth = 150.0f;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - comboWidth));
        ImGui::SetNextItemWidth(comboWidth);
        ImGui::Combo("##lineEnding", &_currentLE, "Newline\0Carriage return\0Both\0");
    }
}

void Console::update() {
    ImGui::PushID("Console");
    ImGui::BeginGroup();

    // Textbox flags
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CtrlEnterForNewLine | ImGuiInputTextFlags_EnterReturnsTrue
        | ImGuiInputTextFlags_AllowTabInput;

    // Textbox (but check for input enable first with short-circuiting &&)
    ImVec2 size{ ImGui::FILL, ImGui::GetTextLineHeight() * Settings::sendTextboxHeight };
    if (_hasInput && ImGui::InputTextMultiline("##input", _textBuf, size, flags)) {
        // Line ending
        const char* endings[] = { "\n", "\r", "\r\n" };
        const char* selectedEnding = endings[_currentLE];

        // InputTextMultiline() always uses \n as a line ending, replace all occurences of \n with the selected ending
        std::string sendString = Strings::replaceAll(_textBuf, "\n", selectedEnding);

        // Add a final line ending if set
        if (_addFinalLineEnding) sendString += selectedEnding;

        // Invoke the callback function if the string is not empty
        if (!sendString.empty()) _inputCallback(sendString);

        if (_clearTextboxOnSend) _textBuf.clear(); // Blank out input textbox
        ImGui::SetItemDefaultFocus();
        ImGui::SetKeyboardFocusHere(-1); // Auto focus on input textbox
    }

    _updateOutput();

    ImGui::EndGroup();
    ImGui::PopID();
}
