// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>

#include <imgui.h>

#include "console.hpp"
#include "os/error.hpp"
#include "utils/task.hpp"

// Manages a textbox and console with config options.
class IOConsole : public Console {
    // State
    bool focusOnTextbox = false; // If keyboard focus is applied to the textbox
    std::string textBuf; // Send textbox buffer

    // Options
    int currentLE = 0; // Index of the line ending selected
    bool sendEchoing = true; // If sent strings are displayed in the output
    bool clearTextboxOnSubmit = true; // If the textbox is cleared when the submit callback is called
    bool addFinalLineEnding = false; // If a final line ending is added to the callback input string
    unsigned int recvSize = 1024; // Unsigned int to work with ImGuiDataType
    unsigned int recvSizeTmp = 1024; // Temporary buffer to hold input

    void drawControls();

public:
    // Draws the window contents and returns text entered into the textbox when Enter is pressed.
    std::optional<std::string> updateWithTextbox();

    unsigned int getRecvSize() {
        return recvSize;
    }

    // Prints the details of a thrown exception.
    Task<> errorHandler(System::SystemError error);
};
