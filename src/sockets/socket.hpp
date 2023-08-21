// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "delegates.hpp"

#include "delegates/closeable.hpp"
#include "utils/task.hpp"

// Represents a socket of any type.
class Socket {
    Delegates::CloseDelegate* _close;
    Delegates::IODelegate* _io;
    Delegates::ClientDelegate* _client;
    Delegates::ConnServerDelegate* _connServer;
    Delegates::DgramServerDelegate* _dgramServer;

public:
    // Constructs an object with delegates.
    Socket(Delegates::CloseDelegate& close, Delegates::IODelegate& io, Delegates::ClientDelegate& client,
           Delegates::ConnServerDelegate& connServer, Delegates::DgramServerDelegate& dgramServer) :
        _close(&close),
        _io(&io), _client(&client), _connServer(&connServer), _dgramServer(&dgramServer) {}

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

    Task<> send(const std::string& s) const {
        return _io->send(s);
    }

    Task<RecvResult> recv() const {
        return _io->recv();
    }

    Task<> connect() const {
        return _client->connect();
    }

    const Device& getServer() const {
        return _client->getServer();
    }

    Task<AcceptResult> accept() const {
        return _connServer->accept();
    }

    Task<DgramRecvResult> recvFrom() const {
        return _dgramServer->recvFrom();
    }

    Task<> sendTo(const std::string& addrTo, const std::string& s) const {
        return _dgramServer->sendTo(addrTo, s);
    }
};
