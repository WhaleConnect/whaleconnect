// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional> // std::bind_front()
#include <mutex>
#include <string>
#include <string_view>

#include "console.hpp"
#include "os/async.hpp"
#include "os/net.hpp"
#include "os/socket.hpp"
#include "window.hpp"

// A class to handle a socket connection in a GUI window.
class ConnWindow : public Window {
    Net::DeviceData _data;     // Remote host to connect to
    Socket _socket;            // Internal socket
    bool _pendingRecv = false; // If a receive operation has not yet completed

    std::mutex _outputMutex; // Mutex for access to the console output

    Console _output{ std::bind_front(&ConnWindow::_sendHandler, this) }; // The console output

    // Connects to the server.
    Task<> _connect();

    // Sends a string through the socket.
    Task<> _sendHandler(std::string_view s);

    // Receives a string from the socket and displays it in the console output.
    Task<> _readHandler();

    // Prints the details of a thrown exception.
    void _errorHandler(const System::SystemError& error) {
        // Check for non-fatal errors, then add error line to console
        if (error) _output.addError(error.formatted());
    }

    // Connects to the target server.
    void _init() override { _connect(); }

    // Handles incoming I/O.
    void _beforeUpdate() override;

    // Draws the window contents.
    void _updateContents() override;

public:
    // Sets the window information (title and remote host).
    ConnWindow(const Net::DeviceData& data, std::string_view extraInfo);
};
