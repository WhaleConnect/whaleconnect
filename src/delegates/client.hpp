// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "delegates.hpp"

#include "net/device.hpp"
#include "net/sockethandle.hpp"
#include "utils/task.hpp"

namespace Delegates {
    // Manages operations on client sockets.
    template <auto Tag>
    class Client : public ClientDelegate {
        SocketHandle<Tag>& _handle;
        const Device& _device;

    public:
        Client(SocketHandle<Tag>& handle, const Device& device) : _handle(handle), _device(device) {}

        Task<> connect() override;
    };
}
