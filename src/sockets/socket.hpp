// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "delegates/delegates.hpp"
#include "utils/task.hpp"

// Represents a socket of any type.
class Socket {
    Delegates::CloseDelegate* _close;
    Delegates::IODelegate* _io;
    Delegates::ClientDelegate* _client;
    Delegates::ServerDelegate* _server;

public:
    // Constructs an object with delegates.
    Socket(Delegates::CloseDelegate& close, Delegates::IODelegate& io, Delegates::ClientDelegate& client,
           Delegates::ServerDelegate& server) :
        _close(&close),
        _io(&io), _client(&client), _server(&server) {}

    virtual ~Socket() = default;

    void close() const {
        _close->close();
    }

    bool isValid() const {
        return _close->isValid();
    }

    void cancelIO() const {
        _close->cancelIO();
    }

    Task<> send(std::string_view data) const {
        return _io->send(std::string{ data });
    }

    Task<RecvResult> recv(size_t size) const {
        return _io->recv(size);
    }

    Task<> connect(const Device& device) const {
        return _client->connect(device);
    }

    ServerAddress startServer(const Device& serverInfo) const {
        return _server->startServer(serverInfo);
    }

    Task<AcceptResult> accept() const {
        return _server->accept();
    }

    Task<DgramRecvResult> recvFrom(size_t size) const {
        return _server->recvFrom(size);
    }

    Task<> sendTo(const Device& device, std::string_view data) const {
        return _server->sendTo(device, std::string{ data });
    }
};
