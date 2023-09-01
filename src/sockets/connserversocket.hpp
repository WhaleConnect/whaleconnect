// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "socket.hpp"

#include "delegates/connserver.hpp"
#include "delegates/noops.hpp"
#include "delegates/sockethandle.hpp"
#include "net/enums.hpp"

template <auto Tag>
class ConnServerSocket : public Socket {
    Delegates::SocketHandle<Tag> _handle;
    Delegates::NoopIO _io;
    Delegates::NoopClient _client;
    Delegates::ConnServer<Tag> _connServer{ _handle };
    Delegates::NoopDgramServer _dgramServer;

    void _init(uint16_t port, int backlog);

public:
    explicit ConnServerSocket(uint16_t port, int backlog) : Socket(_handle, _io, _client, _connServer, _dgramServer) {
        _init(port, backlog);
    }
};
