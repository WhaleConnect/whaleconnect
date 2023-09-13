// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "socket.hpp"

#include "delegates/noops.hpp"
#include "delegates/server.hpp"
#include "delegates/sockethandle.hpp"

template <auto Tag>
class ServerSocket : public Socket {
    Delegates::SocketHandle<Tag> _handle;
    Delegates::NoopIO _io;
    Delegates::NoopClient _client;
    Delegates::Server<Tag> _server{ _handle };

public:
    ServerSocket() : Socket(_handle, _io, _client, _server) {}
};
