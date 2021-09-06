// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A class to represent a text panel with an input textbox

#pragma once

#include <vector> // std::vector
#include <sstream> // std::ostringstream
#include <functional> // std::function

#include <imgui/imgui.h>

#include "app/settings.hpp"
#include "util/formatcompat.hpp"
#include "util/strings.hpp"

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
/// A class to represent a scrollable panel of output text with an optional input textbox.
/// </summary>
class Console {
    bool _hasInput; // If the input textbox is displayed
    bool _scrollToEnd = false; // If the console is force-scrolled to the end
    bool _autoscroll = true; // If console autoscrolls when new data is put
    bool _showTimestamps = false; // If timestamps are shown in the output
    bool _showHex = false; // If items are shown in hexadecimal
    bool _clearTextboxOnSend = true; // If the textbox is cleared on each send
    bool _addFinalLineEnding = false; // If a final line ending is added before sending

    std::function<void(const std::string&)> _inputCallback; // Input textbox submit callback function
    std::vector<ConsoleItem> _items; // Items in console output
    std::string _textBuf; // Buffer for the texbox
    int _currentLE = 0; // The index of the line ending selected

    /// <summary>
    /// Add text to the console. Does not make it go on its own line.
    /// </summary>
    /// <param name="s">The string to add to the output</param>
    /// <param name="color">The color of the text (optional, default is uncolored)</param>
    /// <param name="canUseHex">If the string gets displayed as hexadecimal when set (optional, default is true)</param>
    void _add(const std::string& s, ImVec4 color, bool canUseHex);

    /// <summary>
    /// Draw all elements, excluding the textbox.
    /// </summary>
    void _updateOutput();

public:
    /// <summary>
    /// Console constructor, disables the input textbox.
    /// </summary>
    Console() : _hasInput(false), _inputCallback([](const std::string&) {}) {}

    /// <summary>
    /// Console constructor, set the text input callback function (enables the input textbox).
    /// </summary>
    /// <typeparam name="Fn">A function which takes a string which is the textbox contents at the time</typeparam>
    /// <param name="fn">The function to call when the input textbox contents are submitted</param>
    template <class Fn>
    Console(Fn&& fn) : _hasInput(true), _inputCallback(fn) {}

    /// <summary>
    /// Draw the console output.
    /// </summary>
    void update();

    /// <summary>
    /// Add text to the console. Accepts multi-line strings (i.e. strings with multiple newlines)
    /// </summary>
    /// <param name="s">The string to add to the output</param>
    /// <param name="color">The color of the text (optional, default is uncolored)</param>
    /// <param name="prefix">A string to add before each line (optional, default is none)</param>
    /// <param name="canUseHex">If the string gets displayed as hexadecimal when set (optional, default is true)</param>
    void addText(const std::string& s, const std::string& prefix = "", ImVec4 color = {}, bool canUseHex = true) {
        // Split the string by the '\n' character to get each line
        std::vector<std::string> lines = Strings::split(s, '\n');

        // Add each line
        for (const auto& line : lines) _add(prefix + line, color, canUseHex);
    }

    /// <summary>
    /// Add a red error message.
    /// </summary>
    /// <param name="s">The string to add to the output</param>
    void addError(const std::string& s) {
        // Error messages in red
        forceNextLine();
        addText(s, "[ERROR] ", {1.0f, 0.4f, 0.4f, 1.0f}, false);
        forceNextLine();
    }

    /// <summary>
    /// Add a yellow information message.
    /// </summary>
    /// <param name="s">The string to add to the output</param>
    void addInfo(const std::string& s) {
        // Information in yellow
        forceNextLine();
        addText(s, "[INFO ] ", { 1.0f, 0.8f, 0.6f, 1.0f }, false);
        forceNextLine();
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
