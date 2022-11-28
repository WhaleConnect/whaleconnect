// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "utils/task.hpp"

// Abstract class to represent something that can be closed.
struct Closeable {
    virtual ~Closeable() = default;

    virtual void close() = 0;
};

// Abstract class to represent something that can perform async I/O.
struct Writable : virtual Closeable {
protected:
    static constexpr size_t _recvLen = 1024;

public:
    ~Writable() override = default;

    // Sends a string through the socket.
    // The data is passed as a string (not a string_view) to make a copy and prevent dangling pointers in the coroutine.
    [[nodiscard]] virtual Task<> send(std::string data) const = 0;

    // Receives a string from the socket.
    [[nodiscard]] virtual Task<std::string> recv() const = 0;
};

// Abstract class to represent something that can be connected.
struct Connectable : virtual Writable {
    ~Connectable() override = default;

    [[nodiscard]] virtual Task<> connect() const = 0;
};
