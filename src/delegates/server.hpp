// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>

#include "delegates.hpp"
#include "sockethandle.hpp"
#include "traits.hpp"

#include "net/enums.hpp"

namespace Delegates {
    template <auto Tag>
    class Server : public ServerDelegate {
        SocketHandle<Tag>& _handle;
        ConnectionType _type;
        Traits::Server<Tag> _traits;
        std::map<std::string, sockaddr_storage, std::less<>> _udpClients;

        static std::invalid_argument _unsupported() {
            return std::invalid_argument{ "Operation not supported with socket type" };
        }

    public:
        Server(SocketHandle<Tag>& handle, ConnectionType type, const Traits::Server<Tag>& traits) :
            _handle(handle), _type(type), _traits(traits) {}

        ServerAddress startServer(uint16_t port) override;

        Task<AcceptResult> accept() override;

        Task<DgramRecvResult> recvFrom(size_t) override {
            co_return {};
        }

        Task<> sendTo(std::string, std::string) override {
            co_return;
        }
    };
}

// There are no connectionless operations on Bluetooth sockets

template <>
inline Task<DgramRecvResult> Delegates::Server<SocketTag::BT>::recvFrom(size_t) {
    throw _unsupported();
}

template <>
inline Task<> Delegates::Server<SocketTag::BT>::sendTo(std::string, std::string) {
    co_return throw _unsupported(); // co_return to indicate this is a coroutine and strings should be passed by value
}
