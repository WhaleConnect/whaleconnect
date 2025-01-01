// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
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

    // Device comparator functor for std::map. Using a struct to avoid -Wsubobject-linkage on GCC.
    struct CompDevices {
        bool operator()(const Device& d1, const Device& d2) const {
            return d1.address < d2.address || d1.port < d2.port;
        }
    };

    SocketPtr socket;
    std::map<Device, Client, CompDevices> clients;
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
