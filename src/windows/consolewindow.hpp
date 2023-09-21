// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <mutex>
#include <string>
#include <string_view>

#include "window.hpp"

#include "gui/console.hpp"
#include "os/error.hpp"

// Manages a textbox and console in a window.
class ConsoleWindow : public Window {
    // State
    bool focusOnTextbox = false; // If keyboard focus is applied to the textbox
    std::string textBuf;         // Send textbox buffer

    // Options
    int currentLE = 0;                // Index of the line ending selected
    bool sendEchoing = true;          // If sent strings are displayed in the output
    bool clearTextboxOnSubmit = true; // If the textbox is cleared when the submit callback is called
    bool addFinalLineEnding = false;  // If a final line ending is added to the callback input string

    Console output; // Console output
    std::mutex outputMutex;

    // Draws the controls under the console output.
    void drawControls();

    // Draws additional options.
    virtual void drawOptions() {
        // May be overridden optionally
    }

    // Called when Enter is hit in the textbox.
    virtual void sendHandler(std::string_view s) = 0;

protected:
    // Draws the window contents.
    void updateConsole(int numLines = 0);

    // Prints the details of a thrown exception.
    void errorHandler(const System::SystemError& error);

    void addInfo(std::string_view s) {
        std::scoped_lock g{ outputMutex };
        output.addInfo(s);
    }

    void addText(std::string_view s) {
        std::scoped_lock g{ outputMutex };
        output.addText(s);
    }

public:
    using Window::Window;
};
