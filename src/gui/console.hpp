// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <format>
#include <functional> // std::function
#include <sstream>    // std::ostringstream
#include <string_view>
#include <vector>

#include <imgui.h>

// A class to represent a text panel with an input textbox.
class Console {
    // A structure representing an item in a console output.
    struct ConsoleItem {
        bool canUseHex;             // If the item gets displayed as hexadecimal when the option is set
        std::string text;           // Text string
        std::ostringstream textHex; // Text in hexadecimal format
        ImVec4 color;               // Color
        std::string timestamp;      // Time added
    };

    // State variables
    bool _scrollToEnd = false;    // If the console is force-scrolled to the end
    bool _focusOnTextbox = false; // If keyboard focus is applied to the textbox

    // Option variables
    bool _autoscroll = true;           // If console autoscrolls when new data is put
    bool _showTimestamps = false;      // If timestamps are shown in the output
    bool _showHex = false;             // If items are shown in hexadecimal
    bool _clearTextboxOnSubmit = true; // If the textbox is cleared when the submit callback is called
    bool _addFinalLineEnding = false;  // If a final line ending is added to the callback input string

    std::function<void(std::string_view)> _inputCallback; // The textbox callback function, called on Enter key

    std::vector<ConsoleItem> _items; // Items in console output
    std::string _textBuf;            // Textbox buffer
    int _currentLE = 0;              // Index of the line ending selected

    // Forces subsequent text to go on a new line.
    void _forceNextLine() {
        // If there are no items, new text will have to be on its own line.
        if (_items.empty()) return;

        std::string& lastItem = _items.back().text;
        if (lastItem.back() != '\n') lastItem += '\n';
    }

    // Adds text to the console. Does not make it go on its own line.
    void _add(std::string_view s, const ImVec4& color, bool canUseHex);

    // Draws the output pane only.
    void _updateOutput();

public:
    // Sets the text input callback function.
    // The function should take a string_view argument.
    //
    // The supplied function is called when the Enter key is pressed in the input textbox. The string passed to the
    // function is the contents of the textbox at the time of the callback.
    template <class Fn>
    explicit Console(const Fn& fn) : _inputCallback(fn) {}

    // Draws the input textbox, output pane, and option selectors.
    void update();

    // Adds text to the console. Accepts multiline strings.
    // The color of the text can be set, as well as an optional string to show before each line.
    // If canUseHex is set to false, the text will never be displayed as hexadecimal.
    void addText(std::string_view s, std::string_view pre = "", const ImVec4& color = {}, bool canUseHex = true);

    // Adds a red error message.
    void addError(std::string_view s) {
        // Error messages in red
        _forceNextLine();
        addText(s, "[ERROR] ", { 1.0f, 0.4f, 0.4f, 1.0f }, false);
        _forceNextLine();
    }

    // Adds a yellow information message.
    void addInfo(std::string_view s) {
        // Information in yellow
        _forceNextLine();
        addText(s, "[INFO ] ", { 1.0f, 0.8f, 0.6f, 1.0f }, false);
        _forceNextLine();
    }
};
