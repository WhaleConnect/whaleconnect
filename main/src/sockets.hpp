// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <utility> // std::pair
#include <string> // std::string

#ifdef _WIN32
// Winsock headers for Windows
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2bth.h>

// Link with Winsock static library
#pragma comment(lib, "ws2_32.lib")
#else
#include <cerrno> // errno
#include <cstring> // std::strerror()

// Bluetooth headers for Unix
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include <netinet/in.h> // sockaddr, sockaddr_in, sockaddr_in6
#include <sys/socket.h> // Socket definitions
#include <netdb.h> // addrinfo/getaddrinfo() related indentifiers
#include <unistd.h> // close()
#include <fcntl.h> // fcntl()
#include <poll.h> // poll()

// Remap Winsock functions to Unix equivalents
#define closesocket close
#define WSAPoll poll

// Socket errors
#define WSAEWOULDBLOCK EWOULDBLOCK
#define WSAEINPROGRESS EINPROGRESS

// Bluetooth definitions
#define AF_BTH AF_BLUETOOTH
#define BTHPROTO_RFCOMM BTPROTO_RFCOMM

#define SD_BOTH SHUT_RDWR

// Winsock error status codes
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define NO_ERROR 0

// Equivalent typedefs
typedef sockaddr_rc SOCKADDR_BTH;
typedef int SOCKET;
#endif

#include "util.hpp"

/// <summary>
/// Variables and functions to use to send/receive data through a network socket.
/// </summary>
namespace Sockets {
	/// <summary>
	/// Get the last socket error encountered by the system.
	/// </summary>
	/// <returns>Last error code (int) and detailed error message (string)</returns>
	std::pair<int, std::string> getLastErr();

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
