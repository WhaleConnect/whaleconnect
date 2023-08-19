// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include "sockets/delegates.hpp"
#include "sockets/traits/connserver.hpp"
#include "sockets/traits/sockethandle.hpp"

class Socket;

namespace Delegates {
    template <auto Tag>
    class ConnServer : public ConnServerDelegate {
        using Handle = Traits::SocketHandleType<Tag>;

        Handle& _handle;
        Traits::ConnServer<Tag>& _traits;

    public:
        ConnServer(Handle& handle, Traits::ConnServer<Tag>& traits) : _handle(handle), _traits(traits) {}

        Task<SocketPtr> accept() override;
    };
}
