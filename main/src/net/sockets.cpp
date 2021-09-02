// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdexcept> // std::out_of_range

#ifdef _WIN32
// Winsock headers for Windows
#include <ws2tcpip.h>
#include <ws2bth.h>

#pragma comment(lib, "Ws2_32.lib")

// These don't exist on Windows
#define EAI_SYSTEM 0
#define MSG_NOSIGNAL 0

// Unix int types
typedef ULONG nfds_t;
typedef int socklen_t;
#else
#include <cerrno> // errno

// Bluetooth headers for Linux
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include <netinet/in.h> // sockaddr
#include <sys/socket.h> // Socket definitions
#include <netdb.h> // addrinfo/getaddrinfo() related indentifiers
#include <unistd.h> // close()
#include <fcntl.h> // fcntl()
#include <poll.h> // poll()

#include "btutils.hpp"

// Windows API functions remapped to Berkeley Sockets API functions
#define WSAPoll poll
#define GetAddrInfoW getaddrinfo
#define FreeAddrInfoW freeaddrinfo

// Socket errors
#define WSAEWOULDBLOCK EWOULDBLOCK
#define WSAEINPROGRESS EINPROGRESS

// Bluetooth definitions
#define AF_BTH AF_BLUETOOTH
#define BTHPROTO_RFCOMM BTPROTO_RFCOMM

typedef addrinfo ADDRINFOW;
#endif

#include "sockets.hpp"
#include "util/winutf8.hpp"

int Sockets::getSocketErr(SOCKET sockfd) {
    int err = NO_ERROR; // Initialized in case `getsockopt()` doesn't return anything
    int errlen = sizeof(err);
    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &errlen);
    return err;
}

int Sockets::getLastErr() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

void Sockets::setLastErr(int err) {
#ifdef _WIN32
    WSASetLastError(err);
#else
    errno = err;
#endif
}

Sockets::NamedError Sockets::getErr(int code) {
    try {
        // Attempt to get the element specified by the given code.
        return errors.at(code);
    } catch (const std::out_of_range&) {
        // std::unordered_map::at() throws an exception when the key is invalid.
        // This means the error code is not contained in the data structure and no NamedError corresponds to it.
        return { "UNKNOWN ERROR CODE", "No string is implemented for this error code." };
    }
}

bool Sockets::isFatal(int code, bool allowNonBlock) {
    // Check if the code is actually an error
    if (code == NO_ERROR) return false;

    if (allowNonBlock) {
        // These two errors indicate that a non-blocking operation can't complete right now and are non-fatal
        // Tell the calling function that there's no error, and it should check back later.
        if ((code == WSAEINPROGRESS) || (code == WSAEWOULDBLOCK)) return false;
#ifndef _WIN32
        // EAGAIN on *nix should be treated as the above codes for portability
        if (code == EAGAIN) return false;
#endif
    }

    // The error is fatal
    return true;
}

int Sockets::init() {
#ifdef _WIN32
    // Start Winsock on Windows
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData); // MAKEWORD(2, 2) for Winsock 2.2
#else
    // Start GLib on Linux
    BTUtils::glibInit();
    return NO_ERROR;
#endif
}

void Sockets::cleanup() {
#ifdef _WIN32
    // Cleanup Winsock on Windows
    WSACleanup();
#else
    // Stop GLib on Linux
    BTUtils::glibStop();
#endif
}

/// <summary>
/// Create a non-blocking socket.
/// </summary>
/// <param name="af">Socket address family</param>
/// <param name="type">Socket type</param>
/// <param name="proto">Socket protocol</param>
/// <returns>The socket descriptor on success, INVALID_SOCKET on failure</returns>
static SOCKET nonBlockSocket(int af, int type, int proto) {
    // Construct the socket
    SOCKET sockfd = socket(af, type, proto);
    if (sockfd == INVALID_SOCKET) return INVALID_SOCKET;

    // Set the socket's non-blocking mode
    int nonBlockRet;

#ifdef _WIN32
    ULONG mode = 1;
    nonBlockRet = ioctlsocket(sockfd, FIONBIO, &mode);
#else
    int flags = fcntl(sockfd, F_GETFL, 0);
    nonBlockRet = (flags == SOCKET_ERROR) ? SOCKET_ERROR : fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#endif

    if (nonBlockRet == SOCKET_ERROR) {
        // Non-blocking set failed, close the socket
        Sockets::closeSocket(sockfd);
        sockfd = INVALID_SOCKET;
    }
    return sockfd;
}

SOCKET Sockets::createClientSocket(const DeviceData& data) {
    SOCKET sockfd = INVALID_SOCKET; // Socket file descriptor

    if (data.type == Bluetooth) {
        // Bluetooth connection - set up socket
        sockfd = nonBlockSocket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
        if (sockfd == INVALID_SOCKET) return INVALID_SOCKET;

        // Set up server address structure
#ifdef _WIN32
        SOCKADDR_BTH addr{
            .addressFamily = AF_BTH,
            .btAddr = data.btAddr,
            .serviceClassId = RFCOMM_PROTOCOL_UUID,
            .port = data.port
        };
#else
        bdaddr_t bdaddr;
        str2ba(data.address.c_str(), &bdaddr);
        sockaddr_rc addr{ AF_BLUETOOTH, bdaddr, static_cast<uint8_t>(data.port) };
#endif

        // Connect
        connect(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    } else {
        // Internet protocol - Set up hints for getaddrinfo()
        // Ignore "missing initializer" warnings on GCC and clang
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
        bool isTCP = (data.type == TCP);
        ADDRINFOW hints{
            .ai_flags = AI_NUMERICHOST,
            .ai_family = AF_UNSPEC,
            .ai_socktype = (isTCP) ? SOCK_STREAM : SOCK_DGRAM,
            .ai_protocol = (isTCP) ? IPPROTO_TCP : IPPROTO_UDP
        };
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#ifdef __clang__
#pragma clang diagnostic pop
#endif

        // Wide encoding conversions for Windows
        // These are stored in their own variables to prevent them from being temporaries and destroyed later.
        // If we were to call `.c_str()` and then have these destroyed, `.c_str()` would be a dangling pointer.
        widestr addrWide = toWide(data.address);
        widestr portWide = toWide(data.port);

        // Resolve and connect to the IP, getaddrinfo() and GetAddrInfoW() allow both IPv4 and IPv6 addresses
        ADDRINFOW* addr;
        int gaiResult = GetAddrInfoW(addrWide.c_str(), portWide.c_str(), &hints, &addr);
        if (gaiResult == NO_ERROR) {
            // getaddrinfo() succeeded, initialize socket file descriptor with values created by GAI
            sockfd = nonBlockSocket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
            if (sockfd != INVALID_SOCKET) connect(sockfd, addr->ai_addr, static_cast<socklen_t>(addr->ai_addrlen));
            FreeAddrInfoW(addr); // Release the resources
        } else if (gaiResult != EAI_SYSTEM) {
            // The last error can be set to the getaddrinfo() error, the error-checking functions will handle it
            setLastErr(gaiResult);
        }
    }

    // Check if connection failed
    if (isFatal(getLastErr(), true)) {
        // Destroy the socket:
        closeSocket(sockfd);
        sockfd = INVALID_SOCKET;
    }

    return sockfd;
}

void Sockets::closeSocket(SOCKET sockfd) {
    // Can't close an invalid socket
    if (sockfd == INVALID_SOCKET) return;

    // This may reset the last error code to 0, save it first:
    int lastErrBackup = getLastErr();

#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif

    // Restore the last error code:
    setLastErr(lastErrBackup);
}

int Sockets::sendData(SOCKET sockfd, const std::string& data) {
    // Send data through socket, static_cast the string size to avoid MSVC warning C4267
    // ('var': conversion from 'size_t' to 'type', possible loss of data)
    return send(sockfd, data.c_str(), static_cast<int>(data.size()), MSG_NOSIGNAL);

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
    constexpr int bufLen = 1024;
    char buf[bufLen] = "";

    // Receive and check bytes received
    int ret = recv(sockfd, buf, bufLen - 1, MSG_NOSIGNAL);
    if (ret != SOCKET_ERROR) {
        buf[ret] = '\0'; // Add a null char to the end of the buffer
        data = buf;
    }
    return ret;
}

int Sockets::poll(std::vector<pollfd>& pfds, int timeout) {
    return WSAPoll(pfds.data(), static_cast<nfds_t>(pfds.size()), timeout);
}
