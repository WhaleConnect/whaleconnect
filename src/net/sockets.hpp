// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Main definitions, functions, and utilities for network communication

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
// Winsock SOCKET equals int on Linux for socket descriptors
using SOCKET = int;
#endif

/// <summary>
/// Namespace containing functions for handling network sockets.
/// </summary>
namespace Sockets {
    // All possible connection types
    enum class ConnectionType { TCP, UDP, L2CAPSeqPacket, L2CAPStream, L2CAPDgram, RFCOMM, None };

    /// <summary>
    /// Structure containing a device's metadata.
    /// </summary>
    struct DeviceData {
        ConnectionType type = ConnectionType::None; // Server protocol
        std::string name; // Device name (for displaying only, not used in connections)
        std::string address; // Server address (IP address for TCP/UDP, MAC address for Bluetooth)
        uint16_t port = 0; // Server port
    };

    // A vector of DeviceData structs
    using DeviceDataList = std::vector<DeviceData>;

    struct RecvResult {
        size_t bytesRead = 0;
        std::string data;
    };

    class Socket;

    /// <summary>
    /// Check if a given ConnectionType uses IP.
    /// </summary>
    /// <param name="type">The type to check</param>
    /// <returns>If the type is Internet Protocol-based</returns>
    inline bool connectionTypeIsIP(ConnectionType type) {
        using enum ConnectionType;
        return (type == TCP) || (type == UDP);
    }

    /// <summary>
    /// Check if a given ConnectionType uses Bluetooth.
    /// </summary>
    /// <param name="type">The type to check</param>
    /// <returns>If the type is Bluetooth-based</returns>
    inline bool connectionTypeIsBT(ConnectionType type) {
        using enum ConnectionType;
        return (type == L2CAPSeqPacket) || (type == L2CAPStream) || (type == L2CAPDgram) || (type == RFCOMM);
    }

    /// <summary>
    /// Check if a given ConnectionType is None.
    /// </summary>
    /// <param name="type">The type to check</param>
    /// <returns>If the type is None</returns>
    inline bool connectionTypeIsNone(ConnectionType type) {
        return type == ConnectionType::None;
    }

    /// <summary>
    /// Windows only - Prepare the OS sockets for use by the application.
    /// </summary>
    void init();

    /// <summary>
    /// Windows only - Cleanup Winsock.
    /// </summary>
    void cleanup();

    /// <summary>
    /// Create a client socket to connect to a remote server.
    /// </summary>
    /// <param name="data">The target server to connect to</param>
    /// <returns>The newly-created socket</returns>
    Task<Socket> createClientSocket(const DeviceData& data);

    /// <summary>
    /// Close a socket.
    /// </summary>
    /// <param name="sockfd">The file descriptor of the socket</param>
    void closeSocket(SOCKET sockfd);

    /// <summary>
    /// Send a string through the socket.
    /// </summary>
    /// <param name="sockfd">The socket file descriptor to receive from</param>
    /// <param name="data">The data to send through the socket</param>
    /// <returns>A Task to await the operation</returns>
    Task<> sendData(SOCKET sockfd, std::string_view data);

    /// <summary>
    /// Receive a string from the socket.
    /// </summary>
    /// <param name="sockfd">The socket file descriptor to receive from</param>
    /// <returns>The number of bytes read and the string received</returns>
    Task<RecvResult> recvData(SOCKET sockfd);
}

/// <summary>
/// A class to manage a socket file descriptor with RAII.
/// </summary>
/// <remarks>
/// The design for this class is based on `std::unique_ptr`.
/// </remarks>
class Sockets::Socket {
    // The managed file descriptor
    SOCKET _fd = INVALID_SOCKET;

public:
    /// <summary>
    /// Socket default constructor, construct a Socket instance which owns nothing.
    /// </summary>
    Socket() = default;

    /// <summary>
    /// Socket constructor, construct a Socket instance which owns a file descriptor.
    /// </summary>
    /// <param name="fd">The file descriptor of the socket to manage</param>
    explicit Socket(SOCKET fd) : _fd(fd) {}

    // Deleted copy constructor - sockets are always unique
    Socket(const Socket& other) = delete;

    /// <summary>
    /// Socket move constructor, transfer ownership of `other`'s resources.
    /// </summary>
    /// <param name="other">A Socket instance to acquire resources from (will be left in a cleared state)</param>
    Socket(Socket&& other) noexcept : _fd(other.release()) {}

    /// <summary>
    /// Socket destructor, close the managed socket if it's valid.
    /// </summary>
    ~Socket() { reset(); }

    // Deleted copy assignment operator
    Socket& operator=(const Socket& other) = delete;

    /// <summary>
    /// Move assignment operator, transfer ownership of `other`'s resources.
    /// </summary>
    /// <param name="other">A Socket instance to acquire resources from (will be left in a cleared state)</param>
    /// <returns>*this</returns>
    Socket& operator=(Socket&& other) noexcept {
        reset(other.release());

        return *this;
    }

    /// <summary>
    /// Release ownership of the managed socket. The internal file descriptor will be set to INVALID_SOCKET, and the
    /// caller will be responsible for closing the previously-managed socket.
    /// </summary>
    /// <returns>The file descriptor of the previously-managed socket</returns>
    SOCKET release() {
        SOCKET backup = _fd;
        _fd = INVALID_SOCKET;
        return backup;
    }

    /// <summary>
    /// Close the managed socket, then acquire ownership of a new one.
    /// </summary>
    /// <param name="fd">The file descriptor of the new socket to own (by default invalid)</param>
    void reset(SOCKET fd = INVALID_SOCKET) {
        Sockets::closeSocket(_fd);
        _fd = fd;
    }

    /// <summary>
    /// Access the managed socket.
    /// </summary>
    /// <returns>The managed socket file descriptor</returns>
    SOCKET get() const { return _fd; }

    /// <summary>
    /// Check the validity of the managed socket.
    /// </summary>
    /// <returns>
    /// True if the managed socket is valid (i.e. != INVALID_SOCKET), false otherwise
    /// </returns>
    operator bool() const { return _fd != INVALID_SOCKET; }
};
