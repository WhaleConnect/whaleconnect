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

    Handle _handle;
    Delegates::Closeable<Tag> _close{ _handle };

public:
    SocketHandle() : SocketHandle(invalidHandle) {}

    explicit SocketHandle(Handle handle) : _handle(handle) {}

    // Closes the socket.
    ~SocketHandle() {
        _close.close();
    }

    SocketHandle(const SocketHandle&) = delete;

    // Constructs an object and transfers ownership from another object.
    SocketHandle(SocketHandle&& other) noexcept : _handle(other.release()) {}

    SocketHandle& operator=(const SocketHandle&) = delete;

    // Transfers ownership from another object.
    SocketHandle& operator=(SocketHandle&& other) noexcept {
        set(other.release());
        _close = other._close;

        return *this;
    }

    // Closes the current handle and acquires a new one.
    void reset(Handle other = invalidHandle) noexcept {
        _close.close();
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

    // Accesses the close delegate.
    Delegates::Closeable<Tag>& closeDelegate() {
        return _close;
    }
};
