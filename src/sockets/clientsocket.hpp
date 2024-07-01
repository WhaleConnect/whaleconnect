// Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "socket.hpp"
#include "delegates/bidirectional.hpp"
#include "delegates/client.hpp"
#include "delegates/noops.hpp"
#include "delegates/sockethandle.hpp"
#include "net/enums.hpp"

// An outgoing connection.
template <auto Tag>
class ClientSocket : public Socket {
    Delegates::SocketHandle<Tag> handle;
    Delegates::Bidirectional<Tag> io{ handle };
    Delegates::Client<Tag> client{ handle };
    Delegates::NoopServer server;

public:
    ClientSocket() : Socket(handle, io, client, server) {}
};

using ClientSocketIP = ClientSocket<SocketTag::IP>;
using ClientSocketBT = ClientSocket<SocketTag::BT>;
