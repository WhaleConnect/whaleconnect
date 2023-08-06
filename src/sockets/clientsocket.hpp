// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "socket.hpp"

#include "delegates/bidirectional.hpp"
#include "delegates/client.hpp"
#include "delegates/closeable.hpp"
#include "traits/client.hpp"
#include "utils/handleptr.hpp"
#include "utils/task.hpp"

// A socket representing an outgoing connection.
template <auto Tag>
class ClientSocket : public Socket {
    using Handle = Traits::SocketHandleType<Tag>;
    static constexpr auto invalidHandle = Traits::invalidSocketHandle<Tag>();

    Handle _handle = invalidHandle; // Socket handle
    Traits::Client<Tag> _traits;    // Extra data for client sockets

    // Clients support closure, bidirectional communication, and client operations
    Delegates::Closeable<Tag> _close{ _handle };
    Delegates::Bidirectional<Tag> _io{ _handle };
    Delegates::Client<Tag> _client{ _handle, _traits };

    // Creates the socket.
    void _init();

    // Releases ownership of the managed handle.
    void _release() {
        _handle = invalidHandle;
    }

public:
    // Constructs an object with a target device.
    explicit ClientSocket(const Device& device) : Socket(_close, _io, _client), _traits{ .device = device } {
        _init();
    }

    // Closes the socket.
    ~ClientSocket() override {
        close();
    }

    ClientSocket(const ClientSocket&) = delete;

    // Constructs an object, and transfers ownership from another object.
    ClientSocket(ClientSocket&& other) noexcept : _handle(other._release()) {}

    ClientSocket& operator=(const Socket&) = delete;

    // Transfers ownership from another object.
    ClientSocket& operator=(ClientSocket&& other) noexcept {
        close();
        _handle = other._release();

        return *this;
    }
};
