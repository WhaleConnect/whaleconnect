// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "consolewindow.hpp"

#include "net/device.hpp"
#include "sockets/socket.hpp"

// Handles a socket connection in a GUI window.
class ConnWindow : public ConsoleWindow {
    friend class ConnServerWindow;

    std::unique_ptr<Socket> _socket; // Internal socket
    bool _connected = false;
    bool _pendingRecv = false;

    // Connects to the server.
    Task<> _connect();

    // Sends a string through the socket.
    Task<> _sendHandlerAsync(std::string s);

    // Receives a string from the socket and displays it in the console output.
    Task<> _readHandler();

    // Connects to the target server.
    void _init() override {
        _connect();
    }

    void _sendHandler(std::string_view s) override {
        _sendHandlerAsync(std::string{ s });
    }

    // Handles incoming I/O.
    void _onBeforeUpdate() override;

    void _onUpdate() override {
        _updateConsole();
    }

public:
    // Sets the window information (title and remote host).
    ConnWindow(std::unique_ptr<Socket>&& socket, const Device& device, std::string_view extraInfo);

    // Cancels pending socket I/O.
    ~ConnWindow() override {
        _socket->cancelIO();
    }
};
