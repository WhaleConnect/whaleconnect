// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>

#include "delegates.hpp"
#include "sockethandle.hpp"
#include "utils/task.hpp"

namespace Delegates {
    // Manages bidirectional communication on a socket.
    template <auto Tag>
    class Bidirectional : public IODelegate {
        SocketHandle<Tag>& handle;

    public:
        explicit Bidirectional(SocketHandle<Tag>& handle) : handle(handle) {}

        Task<> send(std::string data) override;

        Task<RecvResult> recv(std::size_t size) override;
    };
}
