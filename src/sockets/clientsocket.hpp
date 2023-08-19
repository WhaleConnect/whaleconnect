// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "enums.hpp"
#include "socket.hpp"
#include "sockethandle.hpp"

#include "delegates/bidirectional.hpp"
#include "delegates/client.hpp"
#include "delegates/closeable.hpp"
#include "delegates/noopconnserver.hpp"
#include "delegates/noopdgramserver.hpp"
#include "traits/client.hpp"
#include "utils/task.hpp"

// A socket representing an outgoing connection.
template <auto Tag>
class ClientSocket : public Socket {
    using Handle = Traits::SocketHandleType<Tag>;
    static constexpr auto invalidHandle = Traits::invalidSocketHandle<Tag>();

    Handle _handle;              // Socket handle
    Traits::Client<Tag> _traits; // Extra data for client sockets

    // Clients support closure, bidirectional communication, and client operations
    Delegates::Closeable<Tag> _close{ _handle };
    Delegates::Bidirectional<Tag> _io{ _handle };
    Delegates::Client<Tag> _client{ _handle, _traits };
    Delegates::NoopConnServer _connServer;
    Delegates::NoopDgramServer _dgramServer;

    SocketHandle<Tag> _socketHandle{ _close, _handle };

    // Creates the socket.
    void _init();

public:
    // Constructs an object with a target device.
    explicit ClientSocket(const Device& device) :
        Socket(_close, _io, _client, _connServer, _dgramServer), _traits{ .device = device } {
        _init();
    }
};

using ClientSocketIP = ClientSocket<SocketTag::IP>;
using ClientSocketBT = ClientSocket<SocketTag::BT>;
