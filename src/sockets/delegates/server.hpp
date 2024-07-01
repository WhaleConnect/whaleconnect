// Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <string>
#include <utility>

#include "delegates.hpp"
#include "sockethandle.hpp"
#include "traits.hpp"
#include "net/device.hpp"
#include "net/enums.hpp"
#include "utils/task.hpp"

namespace Delegates {
    // Manages operations on server sockets.
    template <auto Tag>
    class Server : public ServerDelegate {
        SocketHandle<Tag>& handle;
        NO_UNIQUE_ADDRESS Traits::Server<Tag> traits;

    public:
        explicit Server(SocketHandle<Tag>& handle) : handle(handle) {}

        ServerAddress startServer(const Device& serverInfo) override;

        Task<AcceptResult> accept() override;

        Task<DgramRecvResult> recvFrom(std::size_t size) override;

        Task<> sendTo(Device device, std::string data) override;
    };
}

template <>
ServerAddress Delegates::Server<SocketTag::BT>::startServer(const Device& serverInfo);

template <>
Task<AcceptResult> Delegates::Server<SocketTag::BT>::accept();

// There are no connectionless operations on Bluetooth sockets

template <>
inline Task<DgramRecvResult> Delegates::Server<SocketTag::BT>::recvFrom(std::size_t) {
    std::unreachable();
}

template <>
inline Task<> Delegates::Server<SocketTag::BT>::sendTo(Device, std::string) {
    std::unreachable();
}
