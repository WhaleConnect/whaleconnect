// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "socket.hpp"

#include "delegates/bidirectional.hpp"
#include "delegates/client.hpp"
#include "delegates/closeable.hpp"
#include "delegates/noops.hpp"
#include "net/enums.hpp"
#include "net/sockethandle.hpp"
#include "utils/task.hpp"

// A socket representing an outgoing connection.
template <auto Tag>
class ClientSocket : public Socket {
    SocketHandle<Tag> _handle;

    Delegates::Bidirectional<Tag> _io{ _handle };
    Delegates::Client<Tag> _client;
    Delegates::NoopConnServer _connServer;
    Delegates::NoopDgramServer _dgramServer;

public:
    // Constructs an object with a target device.
    explicit ClientSocket(const Device& device) :
        Socket(_handle.closeDelegate(), _io, _client, _connServer, _dgramServer), _client(_handle, device) {}
};

using ClientSocketIP = ClientSocket<SocketTag::IP>;
using ClientSocketBT = ClientSocket<SocketTag::BT>;
