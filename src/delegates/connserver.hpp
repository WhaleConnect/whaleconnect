// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include "delegates.hpp"
#include "sockethandle.hpp"

class Socket;

namespace Delegates {
    template <auto Tag>
    class ConnServer : public ConnServerDelegate {
        SocketHandle<Tag>& _handle;

    public:
        explicit ConnServer(SocketHandle<Tag>& handle) : _handle(handle) {}

        Task<AcceptResult> accept() override;
    };
}
