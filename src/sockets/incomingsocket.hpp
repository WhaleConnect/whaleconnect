// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "socket.hpp"

#include "delegates/bidirectional.hpp"
#include "delegates/noops.hpp"
#include "delegates/sockethandle.hpp"
#include "delegates/traits.hpp"

template <auto Tag>
class IncomingSocket : public Socket {
    Delegates::SocketHandle<Tag> handle;
    Delegates::Bidirectional<Tag> io{ handle };
    Delegates::NoopClient client;
    Delegates::NoopServer server;

public:
    explicit IncomingSocket(Delegates::SocketHandle<Tag>&& handle) :
        Socket(handle, io, client, server), handle(std::move(handle)) {}
};
