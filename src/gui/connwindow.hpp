// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "console.hpp"
#include "window.hpp"

#include "os/error.hpp"
#include "sockets/device.hpp"
#include "sockets/interfaces.hpp"

// A class to handle a socket connection in a GUI window.
class ConnWindow : public Window {
    std::unique_ptr<Writable> _socket; // Internal socket

    // State
    bool _connected = false;      // If the socket is connected
    bool _pendingRecv = false;    // If a receive operation has not yet completed
    bool _focusOnTextbox = false; // If keyboard focus is applied to the textbox
    std::string _textBuf;         // Send textbox buffer

    // Options
    int _currentLE = 0;                // Index of the line ending selected
    bool _sendEchoing = true;          // If sent strings are displayed in the output
    bool _clearTextboxOnSubmit = true; // If the textbox is cleared when the submit callback is called
    bool _addFinalLineEnding = false;  // If a final line ending is added to the callback input string

    Console _output; // Console output

    // Connects to the server.
    Task<> _connect();

    // Sends a string through the socket.
    Task<> _sendHandler(std::string_view s);

    // Receives a string from the socket and displays it in the console output.
    Task<> _readHandler();

    // Prints the details of a thrown exception.
    void _errorHandler(const System::SystemError& error);

    // Connects to the target server.
    void _init() override {
        _connect();
    }

    // Handles incoming I/O.
    void _onBeforeUpdate() override;

    // Draws the window contents.
    void _onUpdate() override;

    // Draws the controls under the console output.
    void _drawControls();

public:
    // Sets the window information (title and remote host).
    ConnWindow(std::unique_ptr<Writable>&& socket, const Device& device, std::string_view extraInfo);

    // Cancels pending socket I/O.
    ~ConnWindow() override {
        _socket->cancelIO();
    }
};
