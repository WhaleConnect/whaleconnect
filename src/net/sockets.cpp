// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <bit> // std::bit_cast()

#ifdef _WIN32
// Winsock headers
#include <WS2tcpip.h>
#include <ws2bth.h>
#include <MSWSock.h>
#include <mstcpip.h>
#else
// Linux Bluetooth headers
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/l2cap.h>

#include <netinet/in.h> // sockaddr
#include <sys/socket.h> // Socket definitions
#include <unistd.h> // close()
#include <netdb.h> // addrinfo/getaddrinfo() related identifiers
#endif

#include "sockets.hpp"
#include "sys/errcheck.hpp"
#include "sys/handlewrapper.hpp"
#include "util/formatcompat.hpp"
#include "util/strings.hpp"

#ifdef _WIN32
// These don't exist on Windows
constexpr auto EAI_SYSTEM = 0;

// Berkeley sockets int types
using socklen_t = int;
#else
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

void Sockets::init() {
#ifdef _WIN32
    // Start Winsock on Windows
    WSADATA wsaData{};
    CALL_EXPECT_ZERO_RC_ERROR(WSAStartup, MAKEWORD(2, 2), &wsaData); // MAKEWORD(2, 2) for Winsock 2.2
#endif

    Async::init();
}

void Sockets::cleanup() {
    Async::cleanup();

#ifdef _WIN32
    // Cleanup Winsock on Windows
    CALL_EXPECT_ZERO(WSACleanup);
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

static Task<> connectSocket(SOCKET s, const sockaddr* addr, socklen_t addrLen, bool isDgram) {
    // Add the socket to the async queue
    Async::add(s);

#ifdef _WIN32
    // Datagram sockets can be directly connected (ConnectEx() doesn't support them)
    if (isDgram) {
        CALL_EXPECT_ZERO(connect, s, addr, addrLen);
        co_return;
    }

    // ConnectEx() requires the socket to be initially bound.
    // A sockaddr_storage can be used with all connection types, Internet and Bluetooth.
    sockaddr_storage addrBind{ .ss_family = addr->sa_family };

    // The bind() function will work with sockaddr_storage for any address family. However, with Bluetooth, it expects
    // the size parameter to be the size of a Bluetooth address structure. Unlike Internet-based sockets, it will not
    // accept a sockaddr_storage size.
    // This means the size must be spoofed with Bluetooth sockets.
    int addrSize = (addr->sa_family == AF_BTH) ? sizeof(SOCKADDR_BTH) : sizeof(sockaddr_storage);

    // Bind the socket
    CALL_EXPECT_ZERO(bind, s, reinterpret_cast<sockaddr*>(&addrBind), addrSize);

    // Load the ConnectEx() function
    LPFN_CONNECTEX connectExPtr = nullptr;

    GUID guid = WSAID_CONNECTEX;
    DWORD numBytes = 0;
    CALL_EXPECT_ZERO(WSAIoctl,
                     s,
                     SIO_GET_EXTENSION_FUNCTION_POINTER,
                     &guid,
                     sizeof(guid),
                     &connectExPtr,
                     sizeof(connectExPtr),
                     &numBytes,
                     nullptr,
                     nullptr);

    Async::CompletionResult result{};
    co_await result;

    // Call ConnectEx()
    CALL_EXPECT_TRUE(connectExPtr, s, addr, addrLen, nullptr, 0, nullptr, &result);
    co_await std::suspend_always{};
    CALL_EXPECT_ZERO(result.errorResult);

    // Make the socket behave more like a regular socket connected with connect()
    CALL_EXPECT_ZERO(setsockopt, s, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0);
#endif
}

Task<Sockets::Socket> Sockets::createClientSocket(const DeviceData& data) {
    using enum ConnectionType;

    if (connectionTypeIsIP(data.type)) {
        // Internet protocol - Set up hints for getaddrinfo()
        bool isUDP = (data.type == UDP);

        ADDRINFOW hints{
            .ai_flags = AI_NUMERICHOST,
            .ai_family = AF_UNSPEC,
            .ai_socktype = isUDP ? SOCK_DGRAM : SOCK_STREAM,
            .ai_protocol = isUDP ? IPPROTO_UDP : IPPROTO_TCP
        };

        // Wide encoding conversions for Windows
        Strings::SysStr addrWide = Strings::toSys(data.address);
        Strings::SysStr portWide = Strings::toSys(data.port);

        // Resolve and connect to the IP, getaddrinfo() and GetAddrInfoW() allow both IPv4 and IPv6 addresses
        HandleWrapper<ADDRINFOW*> addr{ FreeAddrInfo };
        CALL_EXPECT_ZERO_RC_ERROR_TYPE(GetAddrInfo, System::ErrorType::AddrInfo,
                                       addrWide.c_str(), portWide.c_str(), &hints, &addr.get());

        // Initialize socket
        Socket ret{ CALL_EXPECT_NONERROR(socket, addr->ai_family, addr->ai_socktype, addr->ai_protocol) };

        // Connect to the server
        // The cast to socklen_t is only needed on Windows because ai_addrlen is of type size_t.
        co_await connectSocket(ret.get(), addr->ai_addr, static_cast<socklen_t>(addr->ai_addrlen), isUDP);

        // Return the socket
        co_return std::move(ret);
    } else if (connectionTypeIsBT(data.type)) {
        // Set up Bluetooth socket
        Socket ret{ CALL_EXPECT_NONERROR(bluetoothSocket, data.type) };

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

        bool isDgram = (data.type == L2CAPDgram);

        // Connect, then check if connection failed
        co_await connectSocket(ret.get(), std::bit_cast<sockaddr*>(&sAddrBT), addrSize, isDgram);

        co_return std::move(ret);
    } else {
        // None type
        throw std::invalid_argument{ "None type specified in socket creation" };
    }
}

void Sockets::closeSocket(SOCKET sockfd) {
    // Can't close an invalid socket
    if (sockfd == INVALID_SOCKET) return;

    // This may reset the last error code to 0, save it first:
    int lastErrBackup = System::getLastError();

#ifdef _WIN32
    shutdown(sockfd, SD_BOTH);
    closesocket(sockfd);
#else
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
#endif

    // Restore the last error code:
    System::setLastError(lastErrBackup);
}

Task<> Sockets::sendData(SOCKET sockfd, std::string_view data) {
    Async::CompletionResult result{};
    co_await result;

#ifdef _WIN32
    std::string sendBuf{ data };
    WSABUF buf{ static_cast<ULONG>(sendBuf.size()), sendBuf.data() };
    CALL_EXPECT_NONERROR(WSASend, sockfd, &buf, 1, nullptr, 0, &result, nullptr);
#else
    int sendRet = send(sockfd, data.data(), data.size(), MSG_NOSIGNAL);
#endif

    co_await std::suspend_always{};
    CALL_EXPECT_ZERO(result.errorResult);

    // Note: Typically, sendto() and recvfrom() are used with a UDP connection.
    // However, these require a sockaddr parameter, which becomes hard to get with getaddrinfo().
    // Without this parameter, the call becomes:
    // sendto(sockfd, data, len, 0, nullptr, 0); (nullptr replaces the sockaddr param)
    // Which is equal to send(sockfd, data, len, 0); (https://linux.die.net/man/2/sendto)
    // And is the same function call for TCP/RFCOMM, also removing the need for an if statement.
    // With send() or without the sockaddr parameter, the socket requires a valid connection.
    // This is why connect() is used with UDP.
}

Task<Sockets::RecvResult> Sockets::recvData(SOCKET sockfd) {
    Async::CompletionResult result{};
    co_await result;

    // Receive and check bytes received
    std::string recvBuf(1024, '\0');

#ifdef _WIN32
    WSABUF buf{ static_cast<ULONG>(recvBuf.size()), recvBuf.data() };
    DWORD flags = 0;
    CALL_EXPECT_NONERROR(WSARecv, sockfd, &buf, 1, nullptr, &flags, &result, nullptr);
#else
    int recvRet = recv(asyncData.sockfd, recvBuf, static_cast<int>(std::ssize(recvBuf)) - 1, MSG_NOSIGNAL);
#endif

    co_await std::suspend_always{};
    CALL_EXPECT_ZERO(result.errorResult);

    recvBuf.resize(result.numBytes);
    co_return RecvResult{ static_cast<ULONG>(result.numBytes), std::move(recvBuf) };
}
