// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
// Winsock headers for Windows
#include <ws2tcpip.h>
#include <ws2bth.h>

#pragma comment(lib, "Ws2_32.lib")

// These don't exist on Windows
constexpr auto EAI_SYSTEM = 0;
constexpr auto MSG_NOSIGNAL = 0;

// Unix int types
using nfds_t = ULONG;
using socklen_t = int;
#else
#include <cerrno> // errno

// Bluetooth headers for Linux
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/l2cap.h>

#include <netinet/in.h> // sockaddr
#include <sys/socket.h> // Socket definitions
#include <netdb.h> // addrinfo/getaddrinfo() related indentifiers
#include <unistd.h> // close()
#include <fcntl.h> // fcntl()
#include <poll.h> // poll()

// Winsock-specific definitions and their Berkeley equivalents
constexpr auto WSAPoll = poll;
constexpr auto GetAddrInfoW = getaddrinfo;
constexpr auto FreeAddrInfoW = freeaddrinfo;
using ADDRINFOW = addrinfo;

// Socket errors
constexpr auto WSAEWOULDBLOCK = EWOULDBLOCK;
constexpr auto WSAEINPROGRESS = EINPROGRESS;
constexpr auto WSAEINVAL = EINVAL;

// Bluetooth definitions
constexpr auto AF_BTH = AF_BLUETOOTH;
constexpr auto BTHPROTO_RFCOMM = BTPROTO_RFCOMM;
constexpr auto BTHPROTO_L2CAP = BTPROTO_L2CAP;
#endif

#include "sockets.hpp"
#include "util/formatcompat.hpp"
#include "util/strings.hpp"

int Sockets::getSocketErr(SOCKET sockfd) {
    int err = NO_ERROR; // Initialized in case `getsockopt()` doesn't modify this variable
    socklen_t errlen = sizeof(err);
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

std::string Sockets::formatErr(int code) {
    const char* formatStr = "{}: {}";

#ifdef _WIN32
    // Message buffer
    constexpr size_t msgSize = 512;
    char msg[msgSize];

    // Get the message text
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, nullptr,
                  code, LocaleNameToLCID(L"en-US", 0), msg, msgSize, nullptr);

    return std::format(formatStr, code, msg);
#else
    // `strerrordesc_np()` (a GNU extension) is used since it doesn't translate the error message. A translation is
    // undesirable since the rest of the app isn't translated either.
    // A negative error value on Linux is most likely a getaddrinfo() error, handle that as well.
    return std::format(formatStr, code, (code >= 0) ? strerrordesc_np(code) : gai_strerror(code));
#endif
}

std::string Sockets::formatLastErr() {
    return formatErr(getLastErr());
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
    // WSAStartup() directly returns the error code, making it inconsistent with the rest of the socket APIs.
    // This function sets the last error based on that code as an improvement.
    WSADATA wsaData;
    int startupRet = WSAStartup(MAKEWORD(2, 2), &wsaData); // MAKEWORD(2, 2) for Winsock 2.2
    setLastErr(startupRet);
    return (startupRet == NO_ERROR) ? NO_ERROR : SOCKET_ERROR;
#else
    return NO_ERROR;
#endif
}

int Sockets::cleanup() {
#ifdef _WIN32
    // Cleanup Winsock on Windows
    return WSACleanup();
#else
    return NO_ERROR;
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

static SOCKET bluetoothSocket(Sockets::ConnectionType type) {
    using enum Sockets::ConnectionType;

    // Determine the socket type
    int sockType = 0;
    switch (type) {
    case L2CAPSeqPacket:
        sockType = SOCK_SEQPACKET;
        break;
    case L2CAPStream:
    case RFCOMM:
        // L2CAP can use a stream-based protocol, RFCOMM is stream-only.
        sockType = SOCK_STREAM;
        break;
    case L2CAPDgram:
        sockType = SOCK_DGRAM;
        break;
    default:
        // Should never get here since this function is used internally.
        return INVALID_SOCKET;
    }

    // Determine the socket protocol
    int sockProto = (type == RFCOMM) ? BTHPROTO_RFCOMM : BTHPROTO_L2CAP;

    // Create the socket
    return nonBlockSocket(AF_BTH, sockType, sockProto);
}

SOCKET Sockets::createClientSocket(const DeviceData& data) {
    SOCKET sockfd = INVALID_SOCKET; // Socket file descriptor

    using enum ConnectionType;

    if (connectionTypeIsIP(data.type)) {
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
        Strings::widestr addrWide = Strings::toWide(data.address);
        Strings::widestr portWide = Strings::toWide(data.port);

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
    } else if (connectionTypeIsBT(data.type)) {
        // Set up Bluetooth socket
        sockfd = bluetoothSocket(data.type);
        if (sockfd == INVALID_SOCKET) return INVALID_SOCKET;

        // Set up server address structure
        socklen_t addrSize;

#ifdef _WIN32
        // Convert the MAC address from string form into integer form
        // This is done by removing all colons in the address string, then parsing the resultant string as an
        // integer in base-16 (which is how a MAC address is structured).
        BTH_ADDR btAddr = std::stoull(Strings::replaceAll(data.address, ":", ""), nullptr, 16);

        SOCKADDR_BTH sAddrBT{ .addressFamily = AF_BTH, .btAddr = btAddr, .port = data.port };
        addrSize = sizeof(sAddrBT);
#else
        // Address of the device to connect to
        bdaddr_t bdaddr;
        str2ba(data.address.c_str(), &bdaddr);

        // The structure used depends on the protocol
        union {
            sockaddr_l2 addrL2;
            sockaddr_rc addrRC;
        } sAddrBT;

        // Set the appropriate sockaddr struct based on the protocol
        if (data.type == RFCOMM) {
            sAddrBT.addrRC = { AF_BLUETOOTH, bdaddr, static_cast<uint8_t>(data.port) };
            addrSize = sizeof(sAddrBT.addrRC);
        } else {
            sAddrBT.addrL2 = { AF_BLUETOOTH, htobs(data.port), bdaddr, 0, 0 };
            addrSize = sizeof(sAddrBT.addrL2);
        }
#endif

        // Connect
        connect(sockfd, reinterpret_cast<sockaddr*>(&sAddrBT), addrSize);
    } else {
        // None type
        setLastErr(WSAEINVAL);
        return sockfd;
    }

    // Check if connection failed
    if (isFatal(getLastErr(), true)) {
        // Free the socket:
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
