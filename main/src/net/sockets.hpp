// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Main definitions, functions, and utilities for network communication

#pragma once

#include <string> // std::string
#include <unordered_map> // std::unordered_map

#ifdef _WIN32
// Winsock header
#include <WinSock2.h>
#else
#include <poll.h> // pollfd

// Error status codes
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define NO_ERROR 0

// Windows-style SOCKET equals int on Unix for file descriptors
typedef int SOCKET;
#endif

/// <summary>
/// Structure containing metadata about a device (type, name, address, port) to provide information on how to connect to
/// it and how to describe it.
/// </summary>
struct DeviceData {
    int type; // Type of connection
    std::string name; // Name of device (Bluetooth only)
    std::string address; // Address of device (IP address for TCP/UDP, MAC address for Bluetooth)
    uint16_t port; // Port/channel of device
    uint64_t btAddr; // Bluetooth address as a 64-bit unsigned integer (used on Windows only)
};

/// <summary>
/// Namespace containing functions for handling network sockets.
/// </summary>
namespace Sockets {
    // Enum of all possible connection types
    enum ConnectionType { TCP, UDP, Bluetooth };

    // String representations of connection types
    inline const char* connectionTypesStr[] = { "TCP", "UDP", "Bluetooth" };

    /// <summary>
    /// A structure to represent an error with a symbolic name and a description.
    /// </summary>
    /// <remarks>
    /// A symbolic name is a human-readable identifier for the error (e.g. "ENOMEM").
    /// A description is a short string describing the error (e.g. "No more memory").
    /// </remarks>
    struct NamedError {
        const char* name; // Symbolic name
        const char* desc; // Description
    };

    // The data structure to act as a lookup table for error codes. It maps numeric codes to NamedErrors.
    // It is an unordered_map to store key/value pairs and to provide constant O(1) access time complexity.
    extern const std::unordered_map<long, NamedError> errors;

    int getSocketErr(SOCKET sockfd);

    /// <summary>
    /// Get the error code of the last socket error.
    /// </summary>
    int getLastErr();

    /// <summary>
    /// Set the last socket error code.
    /// </summary>
    void setLastErr(int err);

    /// <summary>
    /// Get the corresponding NamedError from a numeric code.
    /// </summary>
    /// <param name="code">The numeric error code</param>
    /// <returns>The NamedError describing the error specified by the code</returns>
    NamedError getErr(int code);

    /// <summary>
    /// Check if a given error code is fatal (i.e. should follow in a socket close/termination).
    /// </summary>
    /// <param name="code">The code to check</param>
    /// <param name="allowNonBlock">If "non-blocking in progress" type errors are permitted as non-fatal</param>
    /// <returns>If the code is a fatal socket error</returns>
    bool isFatal(int code, bool allowNonBlock = false);

    /// <summary>
    /// Prepare the OS sockets for use by the application.
    /// </summary>
    /// <returns>Windows only: The return value of WSAStartup()</returns>
    int init();

    /// <summary>
    /// Cleanup Winsock on Windows.
    /// </summary>
    void cleanup();

    /// <summary>
    /// Resolve the remote device and attempt to connect to it. The socket created is non-blocking.
    /// </summary>
    /// <param name="data">The DeviceData structure describing what to connect to and how to do so</param>
    /// <returns>Failure/abort: INVALID_SOCKET, Success: The newly-created socket file descriptor</returns>
    SOCKET createClientSocket(const DeviceData& data);

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
    /// <returns>The value returned from send() - -1: failure, >=1: success</returns>
    int sendData(SOCKET sockfd, const std::string& data);

    /// <summary>
    /// Receive a string from the socket.
    /// </summary>
    /// <param name="sockfd">The socket file descriptor to receive from</param>
    /// <param name="data">The string to hold the data received (passed by reference)</param>
    /// <returns>The value returned from recv() - -1: failure, 0: disconnection of remote host, >= 1: success</returns>
    int recvData(SOCKET sockfd, std::string& data);

    /// <summary>
    /// Poll a set of sockets for events.
    /// </summary>
    /// <param name="pfds">A vector of pollfd structures listing each socket and the events to listen for</param>
    /// <param name="timeout">The maximum milliseconds the function should block for</param>
    /// <returns>The number of sockets which have an event (or 0 if none) on success, SOCKET_ERROR on failure</returns>
    int poll(std::vector<pollfd>& pfds, int timeout);
}
