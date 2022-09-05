// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief A class to handle a socket connection in a GUI window
*/

#pragma once

#include <string>
#include <mutex>
#include <functional> // std::bind_front()
#include <string_view>

#include "console.hpp"
#include "window.hpp"
#include "async/async.hpp"
#include "net/sockets.hpp"

/**
 * @brief A class to handle a socket connection in a GUI window.
*/
class ConnWindow : public Window {
    Sockets::DeviceData _data; // The server to connect to
    Sockets::Socket _socket; // The socket
    bool _connected = false; // If the socket has an active connection to a server
    bool _pendingRecv = false; // If a receive operation has not yet completed

    std::mutex _outputMutex; // The mutex for access to the console output
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

    // Draws the window contents and handles I/O.
    void _updateContents() override;

public:
    /**
     * @brief Sets the window information.
     * @param data The server to connect to
     * @param extraInfo Extra information to display in the window's titlebar
    */
    ConnWindow(const Sockets::DeviceData& data, std::string_view extraInfo);
};
