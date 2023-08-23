// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <utility>

#include "delegates/closeable.hpp"
#include "traits/sockethandle.hpp"

// Move-only class to manage a socket file descriptor with RAII.
template <auto Tag>
class SocketHandle {
    using Handle = Traits::SocketHandleType<Tag>;
    static constexpr auto invalidHandle = Traits::invalidSocketHandle<Tag>();

    Delegates::Closeable<Tag>* _close;
    Handle _handle; // Socket handle

    // Releases ownership of the managed handle.
    Handle _release() {
        return std::exchange(_handle, invalidHandle);
    }

public:
    explicit SocketHandle(Delegates::Closeable<Tag>& close, Handle handle) :
        _close(&close), _handle(handle) {}

    // Closes the socket.
    ~SocketHandle() {
        _close->close();
    }

    SocketHandle(const SocketHandle&) = delete;

    // Constructs an object and transfers ownership from another object.
    SocketHandle(SocketHandle&& other) noexcept : _handle(other._release()) {}

    SocketHandle& operator=(const SocketHandle&) = delete;

    SocketHandle& operator=(Handle other) noexcept {
        _close->close();
        _handle = other;

        return *this;
    }

    // Transfers ownership from another object.
    SocketHandle& operator=(SocketHandle&& other) noexcept {
        this = other._release();
        _close = other._close;

        return *this;
    }
};
