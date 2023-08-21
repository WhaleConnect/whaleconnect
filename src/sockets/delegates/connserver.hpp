// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include "sockets/delegates.hpp"
#include "sockets/traits/sockethandle.hpp"

class Socket;

namespace Delegates {
    template <auto Tag>
    class ConnServer : public ConnServerDelegate {
        using Handle = Traits::SocketHandleType<Tag>;

        Handle& _handle;

    public:
        explicit ConnServer(Handle& handle) : _handle(handle) {}

        Task<AcceptResult> accept() override;
    };
}
