// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
// Winsock headers for Windows
#include <ws2tcpip.h>
#include <ws2bth.h>
#else
// Bluetooth headers for Unix
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include <cerrno> // errno
#include <cstring> // std::strerror()

#include <netinet/in.h> // sockaddr, sockaddr_in, sockaddr_in6
#include <sys/socket.h> // Socket definitions
#include <netdb.h> // addrinfo/getaddrinfo() related indentifiers
#include <unistd.h> // close()
#include <fcntl.h> // fcntl()
#include <poll.h> // poll()

// Remap Winsock functions to Unix equivalents
#define WSAPoll poll

// Socket errors
#define WSAEWOULDBLOCK EWOULDBLOCK
#define WSAEINPROGRESS EINPROGRESS
#define WSAETIMEDOUT ETIMEDOUT

// Bluetooth definitions
#define AF_BTH AF_BLUETOOTH
#define BTHPROTO_RFCOMM BTPROTO_RFCOMM

// Equivalent typedefs
typedef sockaddr_rc SOCKADDR_BTH;
#endif

#include "sockets.hpp"

Sockets::SocketError Sockets::getLastErr() {
    // Get the last error's numeric code
    int lastErr = getLastErrInt();
    std::string errMsg;

    // Get the last error's description
#ifdef _WIN32
    constexpr DWORD msgLen = 1024;
    static char msg[msgLen];
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, nullptr,
        lastErr, 0, msg, msgLen, nullptr);

    errMsg = msg;
#else
    errMsg = std::strerror(lastErr);
#endif
    return { lastErr, errMsg };
}

int Sockets::getLastErrInt() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

void Sockets::setLastErrInt(int err) {
#ifdef _WIN32
    WSASetLastError(err);
#else
    errno = err;
#endif
}

/// <summary>
/// Set the blocking status of a socket.
/// </summary>
/// <param name="sockfd">The socket file descriptor for which to set the blocking state</param>
/// <param name="blocking">If the socket blocks</param>
/// <returns>The return value of the function called internally [Windows: ioctlsocket(), Unix: fcntl()]</returns>
static int setBlocking(SOCKET sockfd, bool blocking) {
#ifdef _WIN32
    ULONG mode = !blocking;
    return ioctlsocket(sockfd, FIONBIO, &mode);
#else
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == SOCKET_ERROR) return SOCKET_ERROR; // If flag read failed, abort
    flags = (blocking) ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return fcntl(sockfd, F_SETFL, flags);
#endif
}

int Sockets::connectWithTimeout(SOCKET sockfd, sockaddr* addr, size_t addrlen) {
    // Make the socket non-blocking, abort if this action fails.
    if (setBlocking(sockfd, false) == SOCKET_ERROR) return SOCKET_ERROR;

    // Start connecting on the non-blocking socket
    connect(sockfd, addr, static_cast<int>(addrlen));

    // Get the last socket error
    int lastErr = getLastErrInt();
    if ((lastErr != WSAEWOULDBLOCK) && (lastErr != WSAEINPROGRESS)) {
        // Check if the last socket error is not (WSA)EWOULDBLOCK or (WSA)EINPROGRESS, these are acceptable errors that
        // may be thrown with a non-blocking socket. If it's anything else, set return code to indicate failure.
        return SOCKET_ERROR;
    } else {
        setLastErrInt(NO_ERROR);

        // Wait for connect to complete (or for the timeout deadline)
        pollfd pfds[] = { { sockfd, POLLOUT, 0 } };

        // If poll returns 0 it timed out indicating a failed connection.
        if (WSAPoll(pfds, 1, Settings::connectTimeout * 1000) == 0) {
            setLastErrInt(WSAETIMEDOUT);
            return SOCKET_ERROR;
        }
    }

    // Make socket blocking again
    return setBlocking(sockfd, true);
}

SOCKET Sockets::createClientSocket(const DeviceData& data) {
    SOCKET sockfd = INVALID_SOCKET; // Socket file descriptor
    bool connectSuccess = false; // If the connection was successful

    if (data.type == Bluetooth) {
        // Bluetooth connection uses SOCKADDR_BTH (sockaddr_rc on Unix)
        SOCKADDR_BTH addr;
        sockfd = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);

        // Set up server address structure
        std::memset(&addr, 0, sizeof(addr));
#ifdef _WIN32
        addr.addressFamily = AF_BTH;
        addr.serviceClassId = RFCOMM_PROTOCOL_UUID;
        addr.port = data.port;
        addr.btAddr = data.btAddr;
#else
        addr.rc_family = AF_BLUETOOTH;
        addr.rc_channel = data.port;
        addr.rc_bdaddr = data.btAddr;
#endif

        // Connect
        connectSuccess = connectWithTimeout(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != SOCKET_ERROR;
    } else {
        // Set up hints for getaddrinfo()
        addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_flags = AI_NUMERICHOST;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = (data.type == TCP) ? SOCK_STREAM : SOCK_DGRAM;
        hints.ai_protocol = (data.type == TCP) ? IPPROTO_TCP : IPPROTO_UDP;

        addrinfo* addr;

        // uint16_t=>char[] conversion with snprintf()
        char portStr[6];
        std::snprintf(portStr, ARRAY_LEN(portStr), "%hu", data.port);

        // Resolve and connect to the IP, getaddrinfo() allows both IPv4 and IPv6 addresses
        int d = getaddrinfo(data.address.c_str(), portStr, &hints, &addr);
        if (d == NO_ERROR) {
            // getaddrinfo() succeeded, initialize socket file descriptor with values created by GAI
            sockfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

            // TCP may need a timeout, UDP does not
            if (data.type == TCP)
                connectSuccess = connectWithTimeout(sockfd, addr->ai_addr, addr->ai_addrlen) != SOCKET_ERROR;
            else
                connectSuccess = connect(sockfd, addr->ai_addr, static_cast<int>(addr->ai_addrlen)) != SOCKET_ERROR;

            // Release the resources
            freeaddrinfo(addr);
        }
    }

    // Check if connection failed
    if (!connectSuccess) {
        // destroySocket() may reset the last error code to 0, save it first:
        int lastErrBackup = getLastErrInt();

        // Destroy the socket:
        destroySocket(sockfd);
        sockfd = INVALID_SOCKET;

        // Restore the last error code:
        setLastErrInt(lastErrBackup);
    }

    return sockfd;
}

void Sockets::destroySocket(SOCKET sockfd) {
if (sockfd != INVALID_SOCKET) {
#ifdef _WIN32
        shutdown(sockfd, SD_BOTH);
        closesocket(sockfd);
#else
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
#endif
    }
}

int Sockets::sendData(SOCKET sockfd, const std::string& data) {
    // Send data through socket, static_cast the string size to avoid MSVC warning C4267
    // ('var': conversion from 'size_t' to 'type', possible loss of data)
    return send(sockfd, data.c_str(), static_cast<int>(data.size()), 0);

    // Note: Typically, sendto() and recvfrom() are used with a UDP connection.
    // However, these require a sockaddr parameter, which becomes hard to get with getaddrinfo().
    // Without this parameter, the call becomes:
    // sendto(sockfd, data, len, 0, nullptr, 0); (nullptr replaces the sockaddr param)
    // Which is equal to send(sockfd, data, len, 0); (https://linux.die.net/man/2/sendto)
    // And is the same function call for TCP/RFCOMM, also removing the need for an if statement.
    // With send() or without the sockaddr parameter, the socket requires a valid connection.
    // This is why connect() is used with UDP.
}

int Sockets::recvData(SOCKET sockfd, std::string& data) {
    char buf[1024] = "";

    // Receive and check bytes received
    int ret = recv(sockfd, buf, ARRAY_LEN(buf) - 1, 0);
    if (ret != SOCKET_ERROR) {
        buf[ret] = '\0'; // Add a null char to the end of the buffer
        data = buf;
    }
    return ret;
}
