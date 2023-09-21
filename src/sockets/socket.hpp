// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "delegates/delegates.hpp"
#include "utils/task.hpp"

// Represents a socket of any type.
class Socket {
    Delegates::HandleDelegate* handle;
    Delegates::IODelegate* io;
    Delegates::ClientDelegate* client;
    Delegates::ServerDelegate* server;

public:
    // Constructs an object with delegates.
    Socket(Delegates::HandleDelegate& handle, Delegates::IODelegate& io, Delegates::ClientDelegate& client,
           Delegates::ServerDelegate& server) :
        handle(&handle),
        io(&io), client(&client), server(&server) {}

    virtual ~Socket() = default;

    void close() const {
        handle->close();
    }

    bool isValid() const {
        return handle->isValid();
    }

    void cancelIO() const {
        handle->cancelIO();
    }

    Task<> send(std::string_view data) const {
        return io->send(std::string{ data });
    }

    Task<RecvResult> recv(size_t size) const {
        return io->recv(size);
    }

    Task<> connect(const Device& device) const {
        return client->connect(device);
    }

    ServerAddress startServer(const Device& serverInfo) const {
        return server->startServer(serverInfo);
    }

    Task<AcceptResult> accept() const {
        return server->accept();
    }

    Task<DgramRecvResult> recvFrom(size_t size) const {
        return server->recvFrom(size);
    }

    Task<> sendTo(const Device& device, std::string_view data) const {
        return server->sendTo(device, std::string{ data });
    }
};
