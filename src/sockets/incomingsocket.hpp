// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <utility>

#include "socket.hpp"
#include "delegates/bidirectional.hpp"
#include "delegates/noops.hpp"
#include "delegates/sockethandle.hpp"

// An incoming connection (one accepted from a server).
template <auto Tag>
class IncomingSocket : public Socket {
    Delegates::SocketHandle<Tag> handle;
    Delegates::Bidirectional<Tag> io{ handle };
    Delegates::NoopClient client;
    Delegates::NoopServer server;

public:
    explicit IncomingSocket(Delegates::SocketHandle<Tag>&& handle) :
        Socket(this->handle, io, client, server), handle(std::move(handle)) {}
};
