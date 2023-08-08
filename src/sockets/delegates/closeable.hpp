// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sockets/delegates.hpp"
#include "sockets/traits/sockethandle.hpp"

namespace Delegates {
    // Manages close operations on a socket.
    template <auto Tag>
    class Closeable : public CloseDelegate {
        using Handle = Traits::SocketHandleType<Tag>;

        const Handle& _handle;

    public:
        explicit Closeable(const Handle& handle) : _handle(handle) {}

        void close() const override;

        bool isValid() const override {
            return _handle != Traits::invalidSocketHandle<Tag>();
        }
    };
}
