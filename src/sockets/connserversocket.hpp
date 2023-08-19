// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "enums.hpp"
#include "socket.hpp"
#include "sockethandle.hpp"

#include "delegates/closeable.hpp"
#include "delegates/connserver.hpp"
#include "delegates/noopclient.hpp"
#include "delegates/noopdgramserver.hpp"
#include "delegates/noopio.hpp"
#include "traits/connserver.hpp"

template <auto Tag>
class ConnServerSocket : public Socket {
    using Handle = Traits::SocketHandleType<Tag>;
    static constexpr auto invalidHandle = Traits::invalidSocketHandle<Tag>();

    Handle _handle;
    Traits::ConnServer<Tag> _traits;

    Delegates::Closeable<Tag> _close{ _handle };
    Delegates::NoopIO _io;
    Delegates::NoopClient _client;
    Delegates::ConnServer<Tag> _connServer{ _handle, _traits };
    Delegates::NoopDgramServer _dgramServer;

    SocketHandle<Tag> _socketHandle{ _close, _handle };

public:
    //
};
