// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <format>
#include <string_view>
#include <vector>

#include <imgui.h>

// A class to represent a text panel output.
class Console {
    // A structure representing an item in a console output.
    struct ConsoleItem {
        bool canUseHex;        // If the item gets displayed as hexadecimal when the option is set
        std::string text;      // Text string
        std::string textHex;   // Text in hexadecimal format
        ImVec4 color;          // Color
        std::string timestamp; // Time added
    };

    // State
    bool _scrollToEnd = false; // If the console is force-scrolled to the end

    // Options
    bool _autoscroll = true;      // If console autoscrolls when new data is put
    bool _showTimestamps = false; // If timestamps are shown in the output
    bool _showHex = false;        // If items are shown in hexadecimal

    std::vector<ConsoleItem> _items; // Items in console output

    // Forces subsequent text to go on a new line.
    void _forceNextLine() {
        // If there are no items, new text will have to be on its own line.
        if (_items.empty()) return;

        std::string& lastItem = _items.back().text;
        if (lastItem.back() != '\n') lastItem += '\n';
    }

    // Adds text to the console. Does not make it go on its own line.
    void _add(std::string_view s, const ImVec4& color, bool canUseHex);

public:
    // Draws widgets for each option for use in a menu.
    void drawOptions();

    // Draws the output pane.
    void update(std::string_view id, const ImVec2& size);

    // Adds text to the console. Accepts multiline strings.
    // The color of the text can be set, as well as an optional string to show before each line.
    // If canUseHex is set to false, the text will never be displayed as hexadecimal.
    void addText(std::string_view s, std::string_view pre = "", const ImVec4& color = {}, bool canUseHex = true);

    // Adds a message with a given color and description.
    void addMessage(std::string_view s, std::string_view desc, const ImVec4& color) {
        _forceNextLine();
        addText(s, std::format("[{}] ", desc), color, false);
        _forceNextLine();
    }

    // Adds a red error message.
    void addError(std::string_view s) {
        // Error messages in red
        _forceNextLine();
        addMessage(s, "ERROR", { 1.0f, 0.4f, 0.4f, 1.0f });
        _forceNextLine();
    }

    // Adds a yellow information message.
    void addInfo(std::string_view s) {
        // Information in yellow
        _forceNextLine();
        addMessage(s, "INFO ", { 1.0f, 0.8f, 0.6f, 1.0f });
        _forceNextLine();
    }

    // Clears the output.
    void clear() {
        _items.clear();
    }
};
