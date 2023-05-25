// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "utils/task.hpp"

// Abstract class to represent a resource that can be closed.
struct Closeable {
    virtual ~Closeable() = default;

    virtual void close() = 0;

    // Checks if this resource is still open.
    explicit virtual operator bool() const = 0;
};

// Abstract class to represent something that can perform async I/O.
struct Writable : virtual Closeable {
protected:
    static constexpr size_t _recvLen = 1024;

public:
    ~Writable() override = default;

    // Sends a string.
    // The data is passed as a string (not a string_view) to make a copy and prevent dangling pointers in the coroutine.
    [[nodiscard]] virtual Task<> send(std::string data) const = 0;

    // Receives a string.
    [[nodiscard]] virtual Task<std::string> recv() const = 0;

    // Cancels all pending I/O.
    virtual void cancelIO() const = 0;
};

// Abstract class to represent something that can be connected.
struct Connectable : virtual Writable {
    ~Connectable() override = default;

    // Connects to a target.
    [[nodiscard]] virtual Task<> connect() const = 0;
};
