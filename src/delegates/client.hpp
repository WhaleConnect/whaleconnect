// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "delegates.hpp"
#include "sockethandle.hpp"

#include "net/device.hpp"
#include "utils/task.hpp"

namespace Delegates {
    // Manages operations on client sockets.
    template <auto Tag>
    class Client : public ClientDelegate {
        SocketHandle<Tag>& _handle;

    public:
        explicit Client(SocketHandle<Tag>& handle) : _handle(handle) {}

        Task<> connect(const Device& device) override;
    };
}
