// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include "delegates.hpp"
#include "sockethandle.hpp"

#include "traits/server.hpp"

namespace Delegates {
    template <auto Tag>
    class ConnServer : public ConnServerDelegate {
        SocketHandle<Tag>& _handle;
        Traits::Server<Tag> _traits;

    public:
        ConnServer(SocketHandle<Tag>& handle, const Traits::Server<Tag>& traits) : _handle(handle), _traits(traits) {}

        Task<AcceptResult> accept() override;
    };
}
