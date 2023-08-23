// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>

#include "window.hpp"

#include "gui/console.hpp"
#include "os/error.hpp"

// Manages a textbox and console in a window.
class ConsoleWindow : public Window {
    // State
    bool _focusOnTextbox = false; // If keyboard focus is applied to the textbox
    std::string _textBuf;         // Send textbox buffer

    // Options
    int _currentLE = 0;                // Index of the line ending selected
    bool _sendEchoing = true;          // If sent strings are displayed in the output
    bool _clearTextboxOnSubmit = true; // If the textbox is cleared when the submit callback is called
    bool _addFinalLineEnding = false;  // If a final line ending is added to the callback input string

    Console _output; // Console output

    // Draws the window contents.
    void _onUpdate() override;

    // Draws the controls under the console output.
    void _drawControls();

    // Called when Enter is hit in the textbox.
    virtual void _sendHandler(std::string_view s) = 0;

protected:
    // Prints the details of a thrown exception.
    void _errorHandler(const System::SystemError& error);

    void _addInfo(std::string_view s) {
        _output.addInfo(s);
    }

    void _addText(std::string_view s) {
        _output.addText(s);
    }

public:
    using Window::Window;
};
