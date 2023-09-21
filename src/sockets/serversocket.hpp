// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "socket.hpp"

#include "delegates/noops.hpp"
#include "delegates/server.hpp"
#include "delegates/sockethandle.hpp"

template <auto Tag>
class ServerSocket : public Socket {
    Delegates::SocketHandle<Tag> handle;
    Delegates::NoopIO io;
    Delegates::NoopClient client;
    Delegates::Server<Tag> server{ handle };

public:
    ServerSocket() : Socket(handle, io, client, server) {}
};
