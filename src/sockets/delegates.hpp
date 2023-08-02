// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <concepts>
#include <optional>
#include <string>

#include "device.hpp"

#include "utils/task.hpp"

namespace Delegates {
    // Manages closure of a socket.
    struct CloseDelegate {
        virtual ~CloseDelegate() = default;

        // Closes the socket.
        virtual void close() const = 0;

        // Checks if the socket is valid.
        virtual bool isValid() const = 0;
    };

    // Manages I/O operations on a socket.
    struct IODelegate {
        virtual ~IODelegate() = default;

        // Sends a string.
        // The data is passed as a string to make a copy and prevent dangling pointers in the coroutine.
        virtual Task<> send(std::string s) const = 0;

        // Receives a string.
        virtual Task<std::optional<std::string>> recv() const = 0;

        // Cancels all pending I/O.
        virtual void cancelIO() const = 0;
    };

    // Manages client operations on a socket.
    struct ClientDelegate {
        virtual ~ClientDelegate() = default;

        // Connects to a host.
        virtual Task<> connect() const = 0;

        // Accesses the connected device information.
        virtual const Device& getServer() const = 0;
    };
}
