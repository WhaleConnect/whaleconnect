// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief A class to handle a socket connection in a GUI window
*/

#pragma once

#include <string>
#include <mutex>
#include <functional> // std::bind()
#include <string_view>

#include "console.hpp"
#include "async/async.hpp"
#include "net/sockets.hpp"

/**
 * @brief A class to handle a socket connection in a GUI window.
*/
class ConnWindow {
    Sockets::Socket _socket; // The socket
    bool _connected = false; // If the socket has an active connection to a server
    bool _pendingRecv = false; // If a receive operation has not yet completed

    std::string _title; // The title of the window
    std::string _windowText; // The text in the window's title bar
    bool _open = true; // If the window is currently open
    std::mutex _outputMutex; // The mutex for access to the console output
    Console _output{ std::bind(&ConnWindow::_sendHandler, this, std::placeholders::_1) }; // The console output

    // Connects to a server.
    Task<> _connect(const Sockets::DeviceData& data);

    // Sends a string through the socket.
    Task<> _sendHandler(std::string_view s);

    // Receives a string from the socket, and displays it in the console output.
    Task<> _readHandler();

    // Prints the details of a thrown exception.
    void _errorHandler(const System::SystemError& error) {
        // Check for non-fatal errors, then add error line to console
        if (error) _output.addError(error.formatted());
    }

public:
    /**
     * @brief Sets the window title and creates the connection.
     * @param data The server to connect to
     * @param title The text to display in the window's titlebar
     * @param extraInfo Extra information to display in the window's titlebar
    */
    ConnWindow(const Sockets::DeviceData& data, std::string_view title, std::string_view extraInfo);

    /**
     * @brief Gets the window title.
     * @return The title
    */
    std::string_view getTitle() const { return _title; }

    /**
     * @brief Checks if the window is open.
     * @return If the window is open
    */
    bool open() const { return _open; }

    /**
     * @brief Redraws the window.
    */
    void update();
};
