// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Main definitions, functions, and utilities for network communication

#pragma once

#include <string> // std::string
#include <vector> // std::vector

#ifdef _WIN32
// Winsock header
#include <WinSock2.h>
#else
#include <poll.h> // pollfd

// Error status codes
constexpr auto INVALID_SOCKET = -1; // An invalid socket descriptor
constexpr auto SOCKET_ERROR = -1; // An error has occurred (returned from a function)
constexpr auto NO_ERROR = 0; // Done successfully (returned from a function)

// Windows-style SOCKET equals int on Unix for file descriptors
using SOCKET = int;
#endif

/// <summary>
/// Namespace containing functions for handling network sockets.
/// </summary>
namespace Sockets {
    // All possible connection types
    enum class ConnectionType { TCP, UDP, L2CAPSeqPacket, L2CAPStream, L2CAPDgram, RFCOMM, None };

    /// <summary>
    /// Structure containing metadata about a device (type, name, address, port).
    /// </summary>
    struct DeviceData {
        ConnectionType type = ConnectionType::None; // Type of connection
        std::string name; // Name of device (Bluetooth only)
        std::string address; // Address of device (IP address for TCP/UDP, MAC address for Bluetooth)
        uint16_t port = 0; // Port/channel of device
    };

    // A vector of DeviceData structs
    using DeviceDataList = std::vector<Sockets::DeviceData>;

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
    /// Get the textual name of a ConnectionType.
    /// </summary>
    /// <param name="type">The ConnectionType</param>
    /// <returns>The textual name as a const char*</returns>
    /// <remarks>
    /// Returning a const char* is safe for this function since there are no local variables that are being returned,
    /// just string literals.
    /// </remarks>
    constexpr const char* connectionTypeToStr(Sockets::ConnectionType type) {
        using enum Sockets::ConnectionType;

        switch (type) {
        case TCP:
            return "TCP";
        case UDP:
            return "UDP";
        case L2CAPSeqPacket:
            return "L2CAP SeqPacket";
        case L2CAPStream:
            return "L2CAP Stream";
        case L2CAPDgram:
            return "L2CAP Datagram";
        case RFCOMM:
            return "RFCOMM";
        case None:
            return "None";
        default:
            return "Unknown";
        }
    }

    /// <summary>
    /// Get the error that occurred on a given socket descriptor.
    /// </summary>
    /// <param name="sockfd">The socket to check</param>
    /// <returns>The error code (or NO_ERROR if there's no error)</returns>
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
    /// Format an error code into a readable string. Handles both standard errors and getaddrinfo() errors.
    /// </summary>
    /// <param name="code">The error code to format</param>
    /// <returns>The formatted string with the code and description</returns>
    std::string formatErr(int code);

    /// <summary>
    /// Format the last error code into a readable string. Handles both standard errors and getaddrinfo() errors.
    /// </summary>
    /// <returns>The formatted string with the code, symbolic name, and description</returns>
    std::string formatLastErr();

    /// <summary>
    /// Check if a given error code is fatal (i.e. should follow in a socket close/termination).
    /// </summary>
    /// <param name="code">The code to check</param>
    /// <param name="allowNonBlock">If "non-blocking in progress" type errors are permitted as non-fatal</param>
    /// <returns>If the code is a fatal socket error</returns>
    bool isFatal(int code, bool allowNonBlock = false);

    /// <summary>
    /// Windows only - Prepare the OS sockets for use by the application.
    /// </summary>
    /// <returns>NO_ERROR on success, SOCKET_ERROR on failure (use getLastErr() to get the error code)</returns>
    int init();

    /// <summary>
    /// Windows only - Cleanup Winsock.
    /// </summary>
    /// <returns>NO_ERROR on success, SOCKET_ERROR on failure</returns>
    int cleanup();

    /// <summary>
    /// Connect to a remote server. The socket created is non-blocking.
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
