// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string> // std::string

#ifdef _WIN32
// Winsock header
#include <WinSock2.h>
#else
// Error status codes
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define NO_ERROR 0

// Windows-style SOCKET equals plain Unix int for file descriptors
typedef int SOCKET;
#endif

#include "util.hpp"

/// <summary>
/// Functions for handling network sockets. (create, send, receive, destroy)
/// </summary>
namespace Sockets {
    /// <summary>
    /// A structure containing an int and a string to represent a socket error.
    /// </summary>
    struct SocketError {
        int code{}; // Numeric error code of the error
        std::string desc; // Extended error description
    };

    /// <summary>
    /// Get the last socket error encountered by the system.
    /// </summary>
    /// <returns>Last error code (int) and detailed error message (string)</returns>
    SocketError getLastErr();

    /// <summary>
    /// Get the error code of the last socket error.
    /// </summary>
    int getLastErrInt();

    /// <summary>
    /// Set the last socket error code.
    /// </summary>
    void setLastErrInt(int err);

    /// <summary>
    /// Connect a socket to a server with the timeout specified in Settings.
    /// </summary>
    /// <param name="sockfd">The socket file descriptor to perform the connection on</param>
    /// <param name="addr">The sockaddr* instance describing what to connect to</param>
    /// <param name="addrlen">The size of the addr param [use sizeof()]</param>
    /// <returns>Failure/timeout: SOCKET_ERROR, Success: NO_ERROR</returns>
    int connectWithTimeout(SOCKET sockfd, sockaddr* addr, size_t addrlen);

    /// <summary>
    /// Resolve the remote device and attempt to connect to it. If the type param indicates an IP-type connection
    /// (TCP/UDP), it will decide if the address is IPv4 or IPv6. If the type param indicates a Bluetooth connection,
    /// it will connect with the RFCOMM protocol.
    /// </summary>
    /// <param name="data">The DeviceData structure describing what to connect to</param>
    /// <returns>Failure: INVALID_SOCKET, Success: The newly-created socket file descriptor</returns>
    SOCKET createClientSocket(const DeviceData& data);

    /// <summary>
    /// Shutdown both directions (Send and Receive) of a socket and close it.
    /// </summary>
    /// <param name="sockfd">The file descriptor of the socket to close</param>
    void destroySocket(SOCKET sockfd);

    /// <summary>
    /// Send a string of data through the socket. Will return true if sending succeeded, false if it failed.
    /// </summary>
    /// <param name="sockfd">The socket file descriptor to receive from</param>
    /// <param name="data">The data to send through the socket</param>
    /// <returns>The value returned from send() - -1: failure, >=1: success</returns>
    int sendData(SOCKET sockfd, const std::string& data);

    /// <summary>
    /// Receive a string of data through the socket. Will return true if receiving succeeded, false if it failed.
    /// </summary>
    /// <param name="sockfd">The socket file descriptor to receive from</param>
    /// <param name="data">The string to hold the data received (passed by reference)</param>
    /// <returns>The value returned from recv() - -1: failure, 0: disconnection of remote host, >= 1: success</returns>
    int recvData(SOCKET sockfd, std::string& data);
}
