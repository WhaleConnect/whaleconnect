// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A class to represent a text panel with an input textbox

#pragma once

#include <deque> // std::deque
#include <sstream> // std::ostringstream

#include <imgui/imgui.h>

#include "app/settings.hpp"
#include "util/formatcompat.hpp"
#include "util/imguiext.hpp"
#include "util/stringutils.hpp"

/// <summary>
/// A structure representing an item in a Console object's output.
/// </summary>
struct ConsoleItem {
    bool canUseHex; // If the item gets displayed as hexadecimal when the option is set
    std::string text; // The text of the item
    std::ostringstream textHex; // The text of the item, in hexadecimal format
    ImVec4 color; // The color of the item
    std::string timestamp; // The time when the item was added
};

/// <summary>
/// A class to represent a scrollable panel of output text with an input textbox.
/// </summary>
class Console {
    bool _scrollToEnd = false; // If the console is force-scrolled to the end
    bool _autoscroll = true; // If console autoscrolls when new data is put
    bool _showTimestamps = false; // If timestamps are shown in the output
    bool _showHex = false; // If items are shown in hexadecimal
    bool _clearTextboxOnSend = true; // If the textbox is cleared on each send
    bool _addFinalLineEnding = false; // If a final line ending is added before sending

    std::deque<ConsoleItem> _items; // Items in console output
    std::string _textBuf; // Buffer for the texbox
    int _currentLE = 0; // The index of the line ending selected

    /// <summary>
    /// Draw all elements below the textbox.
    /// </summary>
    void _updateOutput();

public:
    /// <summary>
    /// Draw the console output.
    /// </summary>
    /// <typeparam name="Fn">A function which takes a string which is the textbox contents at the time</typeparam>
    /// <param name="fn">The function to call when the input textbox is activated</param>
    template<class Fn>
    void update(Fn&& fn);

    /// <summary>
    /// Add text to the console. Does not make it go on its own line.
    /// </summary>
    /// <param name="s">The string to add to the output</param>
    /// <param name="color">The color of the text (optional, default is uncolored)</param>
    /// <param name="canUseHex">If the string gets displayed as hexadecimal when set (optional, default is true)</param>
    void addText(const std::string& s, ImVec4 color = {}, bool canUseHex = true);

    /// <summary>
    /// Add a red error message. Does make it go on its own line.
    /// </summary>
    /// <param name="s">The string to add to the output</param>
    void addError(const std::string& s) {
        // Error messages in red
        forceNextLine();
        addText(std::format("[ERROR] {}\n", s), { 1.0f, 0.4f, 0.4f, 1.0f }, false);
    }

    /// <summary>
    /// Add a yellow information message. Does make it go on its own line.
    /// </summary>
    /// <param name="s">The string to add to the output</param>
    void addInfo(const std::string& s) {
        // Information in yellow
        forceNextLine();
        addText(std::format("[INFO ] {}\n", s), { 1.0f, 0.8f, 0.6f, 1.0f }, false);
    }

    /// <summary>
    /// Add a newline to the last line of the output (if it doesn't already end with one). This causes the next item to
    /// go on a new line.
    /// </summary>
    void forceNextLine() {
        // If the deque is empty, the item will have to be on its own line.
        if (_items.empty()) return;

        std::string& lastItem = _items.back().text;
        if (lastItem.back() != '\n') lastItem += '\n';
    }

    /// <summary>
    /// Clear the console output.
    /// </summary>
    void clear() {
        _items.clear();
    }
};

template<class Fn>
void Console::update(Fn&& fn) {
    // Textbox flags
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CtrlEnterForNewLine | ImGuiInputTextFlags_EnterReturnsTrue
        | ImGuiInputTextFlags_AllowTabInput;

    // Textbox
    ImVec2 size{ -FLT_MIN, ImGui::GetTextLineHeight() * Settings::sendTextboxHeight };
    if (ImGui::InputTextMultiline("##ConsoleInput", _textBuf, size, flags)) {
        // Line ending
        const char* endings[] = { "\n", "\r", "\r\n" };
        const char* selectedEnding = endings[_currentLE];

        // InputTextMultiline() always uses \n as a line ending, replace all occurences of \n with the selected ending
        std::string sendString = StringUtils::replaceAll(_textBuf, "\n", selectedEnding);

        // Add a final line ending if set
        if (_addFinalLineEnding) sendString += selectedEnding;

        // Invoke the callback function if the string is not empty
        if (sendString != "") fn(sendString);

        if (_clearTextboxOnSend) _textBuf = ""; // Blank out input textbox
        ImGui::SetItemDefaultFocus();
        ImGui::SetKeyboardFocusHere(-1); // Auto focus on input textbox
    }

    _updateOutput();
}
