// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
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

        Task<RecvResult> recv(size_t size) override;
    };
}
