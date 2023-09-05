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
        static constexpr auto _recvLen = 1024; // Receive buffer length
        SocketHandle<Tag>& _handle;

    public:
        explicit Bidirectional(SocketHandle<Tag>& handle) : _handle(handle) {}

        Task<> send(std::string s) override;

        Task<RecvResult> recv(size_t size) override;
    };
}
