// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
// Winsock headers
#include <ws2tcpip.h>
#include <ws2bth.h>
#include <MSWSock.h>
#include <mstcpip.h>

// Link with Winsock static library
#pragma comment(lib, "Ws2_32")

// These don't exist on Windows
constexpr auto EAI_SYSTEM = 0;
constexpr auto MSG_NOSIGNAL = 0;

// Berkeley sockets int types
using socklen_t = int;
#else
// Linux Bluetooth headers
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/l2cap.h>

#include <netinet/in.h> // sockaddr
#include <sys/socket.h> // Socket definitions
#include <unistd.h> // close()
#include <netdb.h> // addrinfo/getaddrinfo() related indentifiers

// Winsock-specific definitions and their Berkeley equivalents
constexpr auto GetAddrInfo = getaddrinfo;
constexpr auto FreeAddrInfo = freeaddrinfo;
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
#include "sys/error.hpp"
#include "util/formatcompat.hpp"
#include "util/strings.hpp"

int Sockets::init() {
#ifdef _WIN32
    // Start Winsock on Windows
    // WSAStartup() directly returns the error code, making it inconsistent with the rest of the socket APIs.
    // This function sets the last error based on that code as an improvement.
    WSADATA wsaData;
    int startupRet = WSAStartup(MAKEWORD(2, 2), &wsaData); // MAKEWORD(2, 2) for Winsock 2.2
    System::setLastErr(startupRet);
    if (startupRet != NO_ERROR) return SOCKET_ERROR;
#endif

    return Async::init();
}

int Sockets::cleanup() {
    Async::cleanup();

#ifdef _WIN32
    // Cleanup Winsock on Windows
    return WSACleanup();
#else
    return NO_ERROR;
#endif
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
    return socket(AF_BTH, sockType, sockProto);
}

static int connectSocket(SOCKET s,
                         Async::AsyncData& asyncData,
                         const sockaddr* addr,
                         socklen_t addrLen,
                         [[maybe_unused]] bool isDgram) {
    // Add the socket to the async queue
    if (Async::add(s) == SOCKET_ERROR) return SOCKET_ERROR;

    auto connectSocket = [&] { return connect(s, addr, addrLen); };

#ifdef _WIN32
    if (isDgram) {
        int connectRet = connectSocket();

        if (connectRet == NO_ERROR) {
            BOOL postRet = PostQueuedCompletionStatus(Async::getCompletionPort(), 0, 0, &asyncData.overlapped);
            return (postRet) ? NO_ERROR : SOCKET_ERROR;
        }
        return connectRet;
    }

    sockaddr_storage addrBind{ .ss_family = addr->sa_family };
    int addrSize = (addr->sa_family == AF_BTH) ? sizeof(SOCKADDR_BTH) : sizeof(sockaddr_storage);

    // ConnectEx() requires a socket to be initially bound.
    if (bind(s, reinterpret_cast<sockaddr*>(&addrBind), addrSize) == SOCKET_ERROR) return SOCKET_ERROR;

    LPFN_CONNECTEX connectExPtr = nullptr;

    GUID guid = WSAID_CONNECTEX;
    DWORD numBytes = 0;
    int loadSuccess = WSAIoctl(s,
                               SIO_GET_EXTENSION_FUNCTION_POINTER,
                               &guid,
                               sizeof(guid),
                               &connectExPtr,
                               sizeof(connectExPtr),
                               &numBytes,
                               nullptr,
                               nullptr);

    if (loadSuccess == SOCKET_ERROR) return SOCKET_ERROR;

    DWORD bytesSent;
    if (!connectExPtr(s, addr, addrLen, nullptr, 0, &bytesSent, &asyncData.overlapped))
        if (System::isFatal(WSAGetLastError())) return SOCKET_ERROR;

    return NO_ERROR;
#else
    return connectSocket();
#endif
}

Sockets::Socket Sockets::createClientSocket(const DeviceData& data, Async::AsyncData& asyncData) {
    using enum ConnectionType;

    if (connectionTypeIsIP(data.type)) {
        // Internet protocol - Set up hints for getaddrinfo()
        bool isUDP = (data.type == UDP);

        ADDRINFOW hints{
            .ai_flags = AI_NUMERICHOST,
            .ai_family = AF_UNSPEC,
            .ai_socktype = (isUDP) ? SOCK_DGRAM : SOCK_STREAM,
            .ai_protocol = (isUDP) ? IPPROTO_UDP : IPPROTO_TCP
        };

        // Wide encoding conversions for Windows
        Strings::WideStr addrWide = Strings::toWide(data.address);
        Strings::WideStr portWide = Strings::toWide(data.port);

        // Resolve and connect to the IP, getaddrinfo() and GetAddrInfoW() allow both IPv4 and IPv6 addresses
        ADDRINFOW* addr;
        int gaiResult = GetAddrInfo(addrWide.c_str(), portWide.c_str(), &hints, &addr);
        if (gaiResult == NO_ERROR) {
            // getaddrinfo() succeeded, initialize socket file descriptor with values created by GAI
            Socket ret{ socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol) };

            // Connect to the server
            // The cast to `socklen_t` is only needed on Windows because `ai_addrlen` is of type `size_t`.
            int connectResult = SOCKET_ERROR;
            if (ret) connectResult = connectSocket(ret.get(), asyncData, addr->ai_addr, static_cast<socklen_t>(addr->ai_addrlen), isUDP);
            FreeAddrInfo(addr); // Release the resources

            if ((connectResult == SOCKET_ERROR) && System::isFatal(System::getLastErr())) return Socket(INVALID_SOCKET);
            return ret;
        } else {
            // The last error can be set to the getaddrinfo() error, the error-checking functions will handle it
            if (gaiResult != EAI_SYSTEM) System::setLastErr(gaiResult);
            return Socket{ INVALID_SOCKET };
        }
    } else if (connectionTypeIsBT(data.type)) {
        // Set up Bluetooth socket
        Socket ret{ bluetoothSocket(data.type) };
        if (!ret) return Socket{ INVALID_SOCKET };

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

        // Connect, then check if connection failed
        if (connectSocket(ret.get(), asyncData, reinterpret_cast<sockaddr*>(&btAddr), addrSize, data.type == L2CAPDgram) == SOCKET_ERROR)
            if (System::isFatal(System::getLastErr())) return Socket{ INVALID_SOCKET };

        return ret;
    } else {
        // None type
        System::setLastErr(WSAEINVAL);
        return Socket{ INVALID_SOCKET };
    }
}

void Sockets::closeSocket(SOCKET sockfd) {
    // Can't close an invalid socket
    if (sockfd == INVALID_SOCKET) return;

    // This may reset the last error code to 0, save it first:
    int lastErrBackup = System::getLastErr();

#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif

    // Restore the last error code:
    System::setLastErr(lastErrBackup);
}

int Sockets::sendData(SOCKET sockfd, std::string_view data) {
    // Send data through socket, static_cast the string size to avoid MSVC warning C4267
    // ('var': conversion from 'size_t' to 'type', possible loss of data)
    return send(sockfd, data.data(), static_cast<int>(data.size()), MSG_NOSIGNAL);

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
    char buf[1024]{};

    // Receive and check bytes received
    int ret = recv(sockfd, buf, static_cast<int>(std::ssize(buf)) - 1, MSG_NOSIGNAL);
    if (ret != SOCKET_ERROR) {
        buf[ret] = '\0'; // Add a null char to the end of the buffer
        data = buf;
    }
    return ret;
}
