// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A class to represent a text panel with an input textbox

#pragma once

#include <sstream> // std::ostringstream
#include <vector> // std::vector

#include <imgui/imgui.h>

#include "util/imguiext.hpp"

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

    std::vector<ConsoleItem> _items; // Items in console output
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
    /// <typeparam name="Fn">The function to call when the input textbox is activated</typeparam>
    /// <param name="fn">A function which takes a string which is the textbox contents at the time</param>
    template<class Fn>
    void update(Fn fn);

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
    void addError(const std::string& s);

    /// <summary>
    /// Add a yellow information message. Does make it go on its own line.
    /// </summary>
    /// <param name="s">The string to add to the output</param>
    void addInfo(const std::string& s);

    /// <summary>
    /// Add a newline to the last line of the output (if it doesn't already end with one). This causes the next item to
    /// go on a new line.
    /// </summary>
    void forceNextLine();

    /// <summary>
    /// Clear the console output.
    /// </summary>
    void clear();
};

template<class Fn>
void Console::update(Fn fn) {
    // Send textbox
    ImGui::SetNextItemWidth(-FLT_MIN); // Make the textbox full width
    if (ImGui::InputText("##ConsoleInput", _textBuf, ImGuiInputTextFlags_EnterReturnsTrue)) {
        // Construct the string to send by adding the line ending to the end of the string
        const char* endings[] = { "", "\n", "\r", "\r\n" };
        std::string sendString = _textBuf + endings[_currentLE];

        if (sendString != "") fn(sendString);

        if (_clearTextboxOnSend) _textBuf = ""; // Blank out input textbox
        ImGui::SetItemDefaultFocus();
        ImGui::SetKeyboardFocusHere(-1); // Auto focus on input textbox
    }

    _updateOutput();
}
