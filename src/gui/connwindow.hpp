// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional> // std::bind_front()
#include <memory>
#include <mutex>
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

    bool _connected = false;   // If the socket is connected
    bool _pendingRecv = false; // If a receive operation has not yet completed
    std::mutex _outputMutex;   // Mutex for access to the console output

    Console _output{ std::bind_front(&ConnWindow::_sendHandler, this) }; // The console output

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

public:
    // Sets the window information (title and remote host).
    ConnWindow(std::unique_ptr<Writable>&& socket, const Device& device, std::string_view extraInfo);

    ~ConnWindow() override {
        _socket->cancelIO();
    }
};
