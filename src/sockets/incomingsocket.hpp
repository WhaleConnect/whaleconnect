// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>

#include "socket.hpp"

#include "delegates/bidirectional.hpp"
#include "delegates/noops.hpp"
#include "delegates/sockethandle.hpp"
#include "traits/sockethandle.hpp"

template <auto Tag>
class IncomingSocket : public Socket {
    Delegates::SocketHandle<Tag> _handle;
    Delegates::Bidirectional<Tag> _io{ _handle };
    Delegates::NoopClient _client;
    Delegates::NoopConnServer _connServer;
    Delegates::NoopDgramServer _dgramServer;

public:
    explicit IncomingSocket(Delegates::SocketHandle<Tag>&& handle) :
        Socket(_handle, _io, _client, _connServer, _dgramServer), _handle(std::move(handle)) {}
};
