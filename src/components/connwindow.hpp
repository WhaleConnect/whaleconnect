// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <string>
#include <string_view>

#include "ioconsole.hpp"
#include "window.hpp"
#include "net/device.hpp"
#include "sockets/delegates/delegates.hpp"
#include "utils/task.hpp"

// Handles a socket connection in a GUI window.
class ConnWindow : public Window {
    SocketPtr socket; // Internal socket
    IOConsole console;
    std::atomic_bool connected = false;
    std::atomic_bool pendingRecv = false;

    // Connects to the server.
    Task<> connect(Device device);

    // Sends a string through the socket.
    Task<> sendHandler(std::string s);

    // Receives a string from the socket and displays it in the console output.
    Task<> readHandler();

    // Handles incoming I/O.
    void onBeforeUpdate() override;

    void onUpdate() override;

public:
    ConnWindow(std::string_view title, bool useTLS, const Device& device, std::string_view);

    ~ConnWindow() override;
};
