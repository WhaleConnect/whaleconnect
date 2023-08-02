// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>

#include "sockets/delegates.hpp"
#include "sockets/traits/sockethandle.hpp"
#include "utils/task.hpp"

namespace Delegates {
    // Manages bidirectional communication on a socket.
    template <auto Tag>
    class Bidirectional : public IODelegate {
        using Handle = Traits::SocketHandleType<Tag>;

        static constexpr auto _recvLen = 1024; // Receive buffer length
        Handle& _handle;

    public:
        explicit Bidirectional(Handle& handle) : _handle(handle) {}

        Task<> send(std::string s) const override;

        Task<std::optional<std::string>> recv() const override;

        void cancelIO() const override;
    };
}
