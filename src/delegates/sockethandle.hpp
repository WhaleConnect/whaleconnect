// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <utility>

#include "delegates.hpp"
#include "traits.hpp"

namespace Delegates {
    // Manages close operations on a socket.
    template <auto Tag>
    class SocketHandle : public CloseDelegate {
        using Handle = Traits::SocketHandleType<Tag>;
        static constexpr auto invalidHandle = Traits::invalidSocketHandle<Tag>();

        Handle _handle;
        bool _closed = false;

        void _closeImpl() const;

    public:
        SocketHandle() : SocketHandle(invalidHandle) {}

        explicit SocketHandle(Handle handle) : _handle(handle) {}

        // Closes the socket.
        ~SocketHandle() override {
            this->close();
        }

        SocketHandle(const SocketHandle&) = delete;

        // Constructs an object and transfers ownership from another object.
        SocketHandle(SocketHandle&& other) noexcept : _handle(other.release()) {}

        SocketHandle& operator=(const SocketHandle&) = delete;

        // Transfers ownership from another object.
        SocketHandle& operator=(SocketHandle&& other) noexcept {
            set(other.release());
            return *this;
        }

        void close() override {
            if (!_closed && this->isValid()) {
                _closeImpl();
                _closed = true;
            }
        }

        bool isValid() override {
            return _handle != Traits::invalidSocketHandle<Tag>();
        }

        void cancelIO() override;

        // Closes the current handle and acquires a new one.
        void reset(Handle other = invalidHandle) noexcept {
            this->close();
            _handle = other;
        }

        // Releases ownership of the managed handle.
        Handle release() {
            return std::exchange(_handle, invalidHandle);
        }

        // Accesses the handle.
        const Handle& operator*() const {
            return _handle;
        }
    };
}
