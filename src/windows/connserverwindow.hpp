// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "consolewindow.hpp"
#include "windowlist.hpp"

#include "delegates/delegates.hpp"
#include "sockets/socket.hpp"
#include "utils/task.hpp"

class ConnServerWindow : public ConsoleWindow {
    SocketPtr _socket;
    bool _pendingAccept = false;

    std::vector<AcceptResult> _unopenedSockets;

    WindowList _connWindows;

    Task<> _accept();

    void _init() override;

    void _onBeforeUpdate() override;

    void _onUpdate() override;

    // Sends data to all connected clients.
    void _sendHandler(std::string_view s) override;

public:
    explicit ConnServerWindow(std::unique_ptr<Socket>&& socket) : ConsoleWindow("asdsa"), _socket(std::move(socket)) {}

    ~ConnServerWindow() override {
        _socket->cancelIO();
    }
};
