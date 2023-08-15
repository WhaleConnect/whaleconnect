// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "delegates.hpp"

#include "delegates/closeable.hpp"
#include "utils/task.hpp"

// Class to manage a socket file descriptor with RAII.
class Socket {
    const Delegates::CloseDelegate& _close;
    const Delegates::IODelegate& _io;
    const Delegates::ClientDelegate& _client;

public:
    // Constructs an object with delegates.
    Socket(const Delegates::CloseDelegate& close, const Delegates::IODelegate& io,
           const Delegates::ClientDelegate& client) :
        _close(close),
        _io(io), _client(client) {}

    virtual ~Socket() = default;

    bool isValid() const {
        return _close.isValid();
    }

    void close() const {
        _close.close();
    }

    Task<> send(const std::string& s) const {
        return _io.send(s);
    }

    Task<RecvResult> recv() const {
        return _io.recv();
    }

    void cancelIO() const {
        _io.cancelIO();
    }

    Task<> connect() const {
        return _client.connect();
    }

    const Device& getServer() const {
        return _client.getServer();
    }
};
