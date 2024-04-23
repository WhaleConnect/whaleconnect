// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <map>
#include <string>

#include "console.hpp"
#include "ioconsole.hpp"
#include "window.hpp"
#include "net/device.hpp"
#include "sockets/delegates/delegates.hpp"
#include "sockets/socket.hpp"
#include "utils/task.hpp"

// Handles a server socket in a GUI window.
class ServerWindow : public Window {
    // Connection-oriented client.
    struct Client {
        SocketPtr socket;
        Console console;
        int colorIndex;
        bool selected = true;
        bool opened = false;
        bool remove = false;
        bool pendingRecv = false;
        bool connected = true;

        Client(SocketPtr&& socket, int colorIndex) : socket(std::move(socket)), colorIndex(colorIndex) {}

        ~Client() {
            if (socket) socket->cancelIO();
        }

        Task<> recv(IOConsole& serverConsole, const Device& device, unsigned int size);
    };

    SocketPtr socket;
    std::map<Device, Client, std::less<>> clients;
    bool isDgram;

    bool pendingIO = false;
    int colorIndex = 0;

    IOConsole console;
    std::string clientsWindowTitle;

    void startServer(const Device& serverInfo);

    // Accepts connection-oriented clients.
    Task<> accept();

    // Receives from datagram-oriented clients.
    Task<> recvDgram();

    // Selects the next color to display clients in.
    void nextColor();

    // Draws the window containing the list of clients.
    void drawClientsWindow();

    void onBeforeUpdate() override;

    void onUpdate() override;

public:
    explicit ServerWindow(std::string_view title, const Device& serverInfo);

    ~ServerWindow() override;
};
