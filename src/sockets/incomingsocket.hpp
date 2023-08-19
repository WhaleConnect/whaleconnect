// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>

#include "socket.hpp"
#include "sockethandle.hpp"

#include "delegates/bidirectional.hpp"
#include "delegates/closeable.hpp"
#include "delegates/noopclient.hpp"
#include "delegates/noopconnserver.hpp"
#include "delegates/noopdgramserver.hpp"
#include "traits/sockethandle.hpp"

template <auto Tag>
class IncomingSocket : public Socket {
    using Handle = Traits::SocketHandleType<Tag>;
    static constexpr auto invalidHandle = Traits::invalidSocketHandle<Tag>();

    Handle _handle;

    Delegates::Closeable<Tag> _close{ _handle };
    Delegates::Bidirectional<Tag> _io{ _handle };
    Delegates::NoopClient _client;
    Delegates::NoopConnServer _connServer;
    Delegates::NoopDgramServer _dgramServer;

    SocketHandle<Tag> _socketHandle{ _close, _handle };

public:
    explicit IncomingSocket(Handle handle) : Socket(_close, _io, _client, _connServer, _dgramServer), _handle(handle) {}
};
