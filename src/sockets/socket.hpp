// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>
#include <utility> // std::exchange()

#include "interfaces.hpp"
#include "traits.hpp"
#include "utils/task.hpp"

// Class to manage a socket file descriptor with RAII.
template <auto Tag>
class Socket : protected SocketTraits<Tag>, virtual public Closeable {
    // TODO: Remove typename in Clang 16 (P0634R3)
    using typename SocketTraits<Tag>::HandleType;
    using SocketTraits<Tag>::invalidHandle;

protected:
    HandleType _handle = invalidHandle;

    // Releases ownership of the managed handle.
    HandleType _release() { return std::exchange(_handle, invalidHandle); }

public:
    // Constructs an object owning nothing.
    Socket() = default;

    // Constructs an object owning a handle.
    explicit Socket(HandleType handle) : _handle(handle) {}

    Socket(const Socket&) = delete;

    // Constructs an object, and transfers ownership from another object.
    Socket(Socket&& other) noexcept : _handle(other._release()) {}

    // Destructs this object and closes the managed handle.
    ~Socket() override { close(); }

    Socket& operator=(const Socket&) = delete;

    // Transfers ownership from another object.
    Socket& operator=(Socket&& other) noexcept {
        close();
        _handle = other._release();

        return *this;
    }

    // Checks the validity of the managed socket.
    explicit operator bool() const { return _handle != invalidHandle; }

    // Closes the managed socket.
    void close() final;
};

// A socket with the sending and receiving operations defined.
template <auto Tag>
struct WritableSocket : virtual Socket<Tag>, virtual Writable {
    ~WritableSocket() override = default;

    [[nodiscard]] Task<> send(std::string data) const override;

    [[nodiscard]] Task<std::string> recv() const override;

    void cancelIO() const override;
};
