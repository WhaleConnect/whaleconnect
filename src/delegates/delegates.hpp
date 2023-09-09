// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "net/device.hpp"
#include "net/enums.hpp"
#include "utils/task.hpp"

class Socket;

using SocketPtr = std::unique_ptr<Socket>;
using RecvResult = std::optional<std::string>;

struct AcceptResult {
    Device device;
    SocketPtr socket;
};

struct DgramRecvResult {
    std::string fromAddr;
    RecvResult data;
};

struct ServerAddress {
    uint16_t port = 0;
    IPType ipType = IPType::None;
};

namespace Delegates {
    // Manages closure of a socket.
    struct CloseDelegate {
        virtual ~CloseDelegate() = default;

        // Closes the socket.
        virtual void close() = 0;

        // Checks if the socket is valid.
        virtual bool isValid() = 0;

        // Cancels all pending I/O.
        virtual void cancelIO() = 0;
    };

    // Manages I/O operations on a socket.
    struct IODelegate {
        virtual ~IODelegate() = default;

        // Sends a string.
        // The data is passed as a string to make a copy and prevent dangling pointers in the coroutine.
        virtual Task<> send(std::string s) = 0;

        // Receives a string.
        virtual Task<RecvResult> recv(size_t size) = 0;
    };

    // Manages client operations on a socket.
    struct ClientDelegate {
        virtual ~ClientDelegate() = default;

        // Connects to a host.
        virtual Task<> connect() = 0;
    };

    // Manages server operations on a socket.
    struct ServerDelegate {
        virtual ~ServerDelegate() = default;

        // Starts the server and returns the server port number.
        virtual ServerAddress startServer(uint16_t port) = 0;

        // Accepts a client connection.
        virtual Task<AcceptResult> accept() = 0;

        // Receives data from a connectionless client.
        virtual Task<DgramRecvResult> recvFrom(size_t size) = 0;

        // Sends data to a connectionless client.
        virtual Task<> sendTo(std::string addrTo, std::string s) = 0;
    };
}
