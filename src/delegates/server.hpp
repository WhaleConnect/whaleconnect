// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include "delegates.hpp"
#include "sockethandle.hpp"

#include "traits/server.hpp"

namespace Delegates {
    template <auto Tag>
    class Server : public ServerDelegate {
        SocketHandle<Tag>& _handle;
        ConnectionType _type;
        Traits::Server<Tag> _traits;

    public:
        Server(SocketHandle<Tag>& handle, ConnectionType type, const Traits::Server<Tag>& traits) :
            _handle(handle), _type(type), _traits(traits) {}

        uint16_t startServer(uint16_t port) override;

        Task<AcceptResult> accept() override;

        Task<DgramRecvResult> recvFrom(size_t) override {
            co_return {};
        }

        Task<> sendTo(std::string, std::string) override {
            co_return;
        }
    };
}
