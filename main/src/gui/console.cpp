// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iomanip> // std::hex, std::uppercase, std::setw(), std::setfill()

// TODO: Remove the #if check once all major compilers implement timezones for <chrono>
#if __cpp_lib_chrono >= 201803L
#include <chrono> // std::chrono
#endif

#include <imgui/imgui.h>

#include "console.hpp"
#include "util/imguiext.hpp"
#include "util/formatcompat.hpp"

void Console::_updateOutput() {
    // Reserve space at bottom for more elements
    static const float reservedSpace = -ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ConsoleOutput", { 0, reservedSpace }, true, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4, 1 }); // Tighten line spacing

    // Add each item in the deque
    for (const auto& i : _items) {
        // Only color tuples with the last value set to 1 are considered
        bool hasColor = (i.color.w == 1.0f);

        // Timestamps
        if (_showTimestamps) {
            ImGui::TextUnformatted(i.timestamp);
            ImGui::SameLine();
        }

        // Apply color if needed
        if (hasColor) ImGui::PushStyleColor(ImGuiCol_Text, i.color);
        ImGui::TextUnformatted((_showHex && i.canUseHex) ? i.textHex.str() : i.text);
        if (hasColor) ImGui::PopStyleColor();
    }

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

        ImGui::Separator();

        ImGui::MenuItem("Clear texbox on send", nullptr, &_clearTextboxOnSend);

        ImGui::MenuItem("Add final line ending", nullptr, &_addFinalLineEnding);

        ImGui::EndPopup();
    }

    // Line ending combobox
    ImGui::SameLine();

    // The code used to calculate where to put the combobox is derived from
    // https://github.com/ocornut/imgui/issues/4157#issuecomment-843197490
    float comboWidth = 150.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - comboWidth));

    ImGui::SetNextItemWidth(comboWidth);
    ImGui::Combo("##lineEnding", &_currentLE, "Newline\0Carriage return\0Both\0");
}

void Console::addText(const std::string& s, ImVec4 color, bool canUseHex) {
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
    std::string timestamp = "";
#endif

    // Text goes on its own line if deque is empty or the last line ends with a newline
    if (_items.empty() || (_items.back().text.back() == '\n')) _items.push_back({ canUseHex, s, {}, color, timestamp });
    else _items.back().text += s; // Text goes on the last line (append)

    // The code block below is located here as a "caching" mechanism - the hex representation is only computed when the
    // item is added. It is simply retreived in the main loop.
    // If this was in `update()` (which would make the code slightly simpler), this would get unnecessarily re-computed
    // every frame - 60 times a second or more.
    if (canUseHex) {
        // Build the hex representation of the string
        // We're appending to the last item's ostringstream (`_items.back()` is guaranteed to exist at this point).
        // We won't need to check for newlines using this approach.
        std::ostringstream& oss = _items.back().textHex;
        for (const unsigned char& c : s) {
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

void Console::addError(const std::string& s) {
    // Error messages in red
    forceNextLine();
    addText(std::format("[ERROR] {}\n", s), { 1.0f, 0.4f, 0.4f, 1.0f }, false);
}

void Console::addInfo(const std::string& s) {
    // Information in yellow
    forceNextLine();
    addText(std::format("[INFO ] {}\n", s), { 1.0f, 0.8f, 0.6f, 1.0f }, false);
}

void Console::forceNextLine() {
    // If the deque is empty, the item will have to be on its own line.
    if (_items.empty()) return;

    std::string& lastItem = _items.back().text;
    if (lastItem.back() != '\n') lastItem += '\n';
}

void Console::clear() {
    _items.clear();
}
