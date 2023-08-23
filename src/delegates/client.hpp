// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "delegates.hpp"

#include "traits/client.hpp"
#include "traits/sockethandle.hpp"
#include "utils/task.hpp"

namespace Delegates {
    // Manages operations on client sockets.
    template <auto Tag>
    class Client : public ClientDelegate {
        using Handle = Traits::SocketHandleType<Tag>;

        Handle& _handle;
        Traits::Client<Tag>& _traits;

    public:
        Client(Handle& handle, Traits::Client<Tag>& traits) : _handle(handle), _traits(traits) {}

        Task<> connect() override;

        const Device& getServer() override {
            return _traits.device;
        }
    };
}
