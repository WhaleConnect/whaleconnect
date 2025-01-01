// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "socket.hpp"
#include "delegates/noops.hpp"
#include "delegates/server.hpp"
#include "delegates/sockethandle.hpp"

// A server that accepts incoming connections.
template <auto Tag>
class ServerSocket : public Socket {
    Delegates::SocketHandle<Tag> handle;
    Delegates::NoopIO io;
    Delegates::NoopClient client;
    Delegates::Server<Tag> server{ handle };

public:
    ServerSocket() : Socket(handle, io, client, server) {}
};
