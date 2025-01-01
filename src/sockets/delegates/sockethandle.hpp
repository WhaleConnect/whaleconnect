// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <utility>

#include "delegates.hpp"
#include "traits.hpp"

namespace Delegates {
    // Manages close operations on sockets.
    template <auto Tag>
    class SocketHandle : public HandleDelegate {
        using Handle = Traits::SocketHandleType<Tag>;
        static constexpr auto invalidHandle = Traits::invalidSocketHandle<Tag>();

        Handle handle;
        bool closed = false;

        void closeImpl();

    public:
        SocketHandle() : SocketHandle(invalidHandle) {}

        explicit SocketHandle(Handle handle) : handle(handle) {}

        // Closes the socket.
        ~SocketHandle() override {
            close();
        }

        SocketHandle(const SocketHandle&) = delete;

        // Constructs an object and transfers ownership from another object.
        SocketHandle(SocketHandle&& other) noexcept : handle(other.release()) {}

        SocketHandle& operator=(const SocketHandle&) = delete;

        // Transfers ownership from another object.
        SocketHandle& operator=(SocketHandle&& other) noexcept {
            reset(other.release());
            return *this;
        }

        void close() override {
            if (!closed && isValid()) {
                closeImpl();
                closed = true;
            }
        }

        bool isValid() override {
            return handle != Traits::invalidSocketHandle<Tag>();
        }

        void cancelIO() override;

        // Closes the current handle and acquires a new one.
        void reset(Handle other = invalidHandle) noexcept {
            close();
            handle = other;
        }

        // Releases ownership of the managed handle.
        Handle release() {
            return std::exchange(handle, invalidHandle);
        }

        // Accesses the handle.
        const Handle& operator*() const {
            return handle;
        }

        Handle& operator*() {
            return handle;
        }
    };
}
