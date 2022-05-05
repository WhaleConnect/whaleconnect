// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief Main definitions, functions, and utilities for socket handling
*/

#pragma once

#include <string>
#include <vector>
#include <string_view>

#include "async/async.hpp"
#include "async/task.hpp"
#include "sys/error.hpp"

#ifdef _WIN32
// Winsock header
#include <WinSock2.h>
#else
using SOCKET = int; /**< Socket type to match code on Windows */
#endif

namespace Sockets {
    /**
     * @brief All possible connection types.
     *
     * L2CAP connections are not supported on Windows because of limitations with the Microsoft Bluetooth stack.
    */
    enum class ConnectionType { TCP, UDP, L2CAPSeqPacket, L2CAPStream, L2CAPDgram, RFCOMM, None };

    /**
     * @brief A remote device's metadata.
    */
    struct DeviceData {
        ConnectionType type = ConnectionType::None; /**< The connection protocol */
        std::string name; /**< The device name for display */
        std::string address; /**< The address (IP address for TCP / UDP, MAC address for Bluetooth) */
        uint16_t port = 0; /**< The port (or PSM for L2CAP, channel for RFCOMM) */
    };

    /**
     * @brief A vector of @p DeviceData structs.
     * @sa DeviceData
    */
    using DeviceDataList = std::vector<DeviceData>;

    /**
     * @brief The results of a receive operation.
    */
    struct RecvResult {
        size_t bytesRead = 0; /**< The number of bytes read */
        std::string data; /**< The string received */
    };

    class Socket;

    /**
     * @brief Checks if a @p ConnectionType is an Internet-based connection.
     * @param type The type to check
     * @return If the connection type is TCP or UDP
    */
    inline bool connectionTypeIsIP(ConnectionType type) {
        using enum ConnectionType;
        return (type == TCP) || (type == UDP);
    }

    /**
     * @brief Checks if a @p ConnectionType is a Bluetooth-based connection.
     * @param type The type to check
     * @return If the connection type is L2CAP or RFCOMM
    */
    inline bool connectionTypeIsBT(ConnectionType type) {
        using enum ConnectionType;
        return (type == L2CAPSeqPacket) || (type == L2CAPStream) || (type == L2CAPDgram) || (type == RFCOMM);
    }

    /**
     * @brief Prepares the OS sockets for use by the application.
    */
    void init();

    /**
     * @brief Cleans up the OS sockets.
    */
    void cleanup();

    /**
     * @brief Creates a client socket and connect to a server.
     * @param data The target server to connect to
     * @return The created socket
    */
    Task<Socket> createClientSocket(const DeviceData& data);

    /**
     * @brief Closes a socket.
     * @param sockfd The socket file descriptor
    */
    void closeSocket(SOCKET sockfd);

    /**
     * @brief Sends a string through a socket.
     * @param sockfd The socket file descriptor
     * @param data The data to send
     * @return A task to await the operation
    */
    Task<> sendData(SOCKET sockfd, std::string_view data);

    /**
     * @brief Receives a string from a socket.
     * @param sockfd The socket file descriptor
     * @return The number of bytes read and the string received
    */
    Task<RecvResult> recvData(SOCKET sockfd);
}

/**
 * @brief A class to manage a socket file descriptor with RAII.
*/
class Sockets::Socket {
    SOCKET _fd = INVALID_SOCKET; // The managed file descriptor

public:
    /**
     * @brief Constructs an object owning nothing.
    */
    Socket() = default;

    /**
     * @brief Constructs an object owning a file descriptor.
     * @param fd The socket file descriptor to take ownership of
    */
    explicit Socket(SOCKET fd) : _fd(fd) {}

    Socket(const Socket& other) = delete;

    /**
     * @brief Constructs an object, and transfers ownership from another object.
     * @param other The object to acquire ownership from
    */
    Socket(Socket&& other) noexcept : _fd(other.release()) {}

    /**
     * @brief Closes the managed socket.
    */
    ~Socket() { reset(); }

    Socket& operator=(const Socket& other) = delete;

    /**
     * @brief Transfers ownership from another object.
     * @param other The object to acquire ownership from
     * @return This object
    */
    Socket& operator=(Socket&& other) noexcept {
        reset(other.release());

        return *this;
    }

    /**
     * @brief Releases ownership of the managed socket.
     * @return The file descriptor of the previously-owned socket
     *
     * The internal socket will be set to @p INVALID_SOCKET, and the caller will be responsible for closing the
     * previously-owned socket.
    */
    SOCKET release() {
        SOCKET backup = _fd;
        _fd = INVALID_SOCKET;
        return backup;
    }

    /**
     * @brief Closes the managed socket, then acquire ownership of a new one.
     * @param fd The new socket file descriptor to own (by default invalid)
    */
    void reset(SOCKET fd = INVALID_SOCKET) {
        Sockets::closeSocket(_fd);
        _fd = fd;
    }

    /**
     * @brief Gets the managed socket.
     * @return The socket file descriptor
    */
    SOCKET get() const { return _fd; }

    /**
     * @brief Checks the validity of the managed socket.
    */
    operator bool() const { return _fd != INVALID_SOCKET; }
};
