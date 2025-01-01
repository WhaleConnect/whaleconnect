// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <format>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <imgui.h>
#include <textselect.hpp>

// Text panel output with colors and other information.
class Console {
    // Item in console output.
    struct ConsoleItem {
        bool canUseHex; // If the item gets displayed as hexadecimal when the option is set
        std::string text; // Text string
        std::string textHex; // Text in hexadecimal format (UTF-8 encoded)
        ImVec4 color; // Color
        std::string timestamp; // Time added
        std::optional<std::string> hoverText; // Tooltip text
    };

    // State
    bool scrollToEnd = false; // If the console is force-scrolled to the end
    float yScrollPos = 0; // Scroll position in vertical axis

    // Options
    bool autoscroll = true; // If console autoscrolls when new data is put
    bool showTimestamps = false; // If timestamps are shown in the output
    bool showHex = false; // If items are shown in hexadecimal

    std::vector<ConsoleItem> items; // Items in console output

    // Text selection manager
    TextSelect textSelect{ std::bind_front(&Console::getLineAtIdx, this),
        std::bind_front(&Console::getNumLines, this) };

    // Forces subsequent text to go on a new line.
    void forceNextLine() {
        // If there are no items, new text will have to be on its own line.
        if (items.empty()) return;

        std::string& lastItem = items.back().text;
        if (!lastItem.ends_with('\n')) lastItem += '\n';
    }

    // Adds text to the console. Does not make it go on its own line.
    void add(std::string_view s, const ImVec4& color, bool canUseHex, std::string_view hoverText);

    // Draws the timestamps to the left of the content.
    void drawTimestamps();

    // Draws the contents of the right-click context menu.
    void drawContextMenu();

    // Draws widgets for each option for use in a menu.
    void drawOptions();

    // Gets the line at an index.
    std::string_view getLineAtIdx(std::size_t i) const {
        const ConsoleItem& item = items[i];
        return showHex && item.canUseHex ? item.textHex : item.text;
    }

    // Gets the number of lines in the output.
    std::size_t getNumLines() const {
        return items.size();
    }

public:
    // Draws the output pane.
    void update(std::string_view id);

    // Adds text to the console. Accepts multiline strings.
    // The color of the text can be set, as well as an optional string to show before each line.
    // If canUseHex is set to false, the text will never be displayed as hexadecimal.
    void addText(std::string_view s, std::string_view pre = "", const ImVec4& color = {}, bool canUseHex = true,
        std::string_view hoverText = "");

    // Adds a message with a given color and description.
    void addMessage(std::string_view s, std::string_view desc, const ImVec4& color) {
        forceNextLine();
        addText(s, std::format("[{}] ", desc), color, false);
        forceNextLine();
    }

    // Adds a red error message.
    void addError(std::string_view s) {
        // Error messages in red
        forceNextLine();
        addMessage(s, "ERROR", { 1.0f, 0.4f, 0.4f, 1.0f });
        forceNextLine();
    }

    // Adds a yellow information message.
    void addInfo(std::string_view s) {
        // Information in yellow
        forceNextLine();
        addMessage(s, "INFO ", { 1.0f, 0.8f, 0.6f, 1.0f });
        forceNextLine();
    }

    // Clears the output.
    void clear() {
        items.clear();
    }
};
