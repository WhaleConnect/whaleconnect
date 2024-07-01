// Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
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
        SocketHandle<Tag>& handle;

    public:
        explicit Client(SocketHandle<Tag>& handle) : handle(handle) {}

        Task<> connect(Device device) override;
    };
}
