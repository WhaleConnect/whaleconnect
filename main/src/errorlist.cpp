// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This file was generated with generate_error_list_cpp.py.
// DO NOT MODIFY THIS FILE, edit the generator script/JSON data instead.

#include "error.hpp"

#ifdef _WIN32
#include <WinSock2.h>
#undef EINTR
#define EINTR WSAEINTR
#undef EACCES
#define EACCES WSAEACCES
#undef EINVAL
#define EINVAL WSAEINVAL
#undef EFAULT
#define EFAULT WSAEFAULT
#undef EADDRNOTAVAIL
#define EADDRNOTAVAIL WSAEADDRNOTAVAIL
#undef ENFILE
#define ENFILE WSAEMFILE
#undef ENOMEM
#define ENOMEM WSAENOBUFS
#undef EMSGSIZE
#define EMSGSIZE WSAEMSGSIZE
#undef ENODATA
#define ENODATA WSANO_DATA
#undef EAGAIN
#define EAGAIN WSAEWOULDBLOCK
#undef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#undef EALREADY
#define EALREADY WSAEALREADY
#undef EDESTADDRREQ
#define EDESTADDRREQ WSAEDESTADDRREQ
#undef EADDRINUSE
#define EADDRINUSE WSAEADDRINUSE
#undef EPROTOTYPE
#define EPROTOTYPE WSAEPROTOTYPE
#undef ENOPROTOOPT
#define ENOPROTOOPT WSAENOPROTOOPT
#undef EPROTONOSUPPORT
#define EPROTONOSUPPORT WSAEPROTONOSUPPORT
#undef ESOCKTNOSUPPORT
#define ESOCKTNOSUPPORT WSAESOCKTNOSUPPORT
#undef EPFNOSUPPORT
#define EPFNOSUPPORT WSAEPFNOSUPPORT
#undef EAFNOSUPPORT
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#undef ENETDOWN
#define ENETDOWN WSAENETDOWN
#undef ENETUNREACH
#define ENETUNREACH WSAENETUNREACH
#undef ENETRESET
#define ENETRESET WSAENETRESET
#undef ECONNABORTED
#define ECONNABORTED WSAECONNABORTED
#undef ECONNRESET
#define ECONNRESET WSAECONNRESET
#undef EISCONN
#define EISCONN WSAEISCONN
#undef ENOTCONN
#define ENOTCONN WSAENOTCONN
#undef ESHUTDOWN
#define ESHUTDOWN WSAESHUTDOWN
#undef EOPNOTSUPP
#define EOPNOTSUPP WSAEOPNOTSUPP
#undef ENOTSOCK
#define ENOTSOCK WSAENOTSOCK
#undef EBADFD
#define EBADFD WSAEBADF
#undef ETIMEDOUT
#define ETIMEDOUT WSAETIMEDOUT
#undef ECONNREFUSED
#define ECONNREFUSED WSAECONNREFUSED
#undef EHOSTDOWN
#define EHOSTDOWN WSAEHOSTDOWN
#undef EHOSTUNREACH
#define EHOSTUNREACH WSAEHOSTUNREACH
#else
#include <cerrno>
#include <netdb.h>
#endif

#define E(code, desc) { code, { #code, desc } }

const std::unordered_map<long, Sockets::NamedError> Sockets::errors = {
    E(EINTR, "An operation was interrupted by a signal."),
    E(EACCES, "Permission denied."),
    E(EINVAL, "An argument is invalid."),
    E(EFAULT, "A supplied internal pointer address is invalid."),
    E(EADDRNOTAVAIL, "The address or port is not valid in its context."),
    E(ENFILE, "Too many open sockets at this time."),
    E(ENOMEM, "No buffer space or memory available."),
    E(EMSGSIZE, "A datagram transferred was larger than the buffer allocated for it."),
    E(ENODATA, "The requested name is valid, but no data of the requested type was found."),
    E(EAGAIN, "A nonblocking operation using a resource cannot complete immediately."),
    E(EINPROGRESS, "Only one blocking operation is permitted at a time."),
    E(EALREADY, "An operation on a nonblocking socket was called, but one is already in progress."),
    E(EDESTADDRREQ, "A required destination address for a send call was omitted."),
    E(EADDRINUSE, "The address is already in use - the address or port may not have been fully closed."),
    E(EPROTOTYPE, "The protocol specified to create a socket does not match its semantics."),
    E(ENOPROTOOPT, "An unknown or invalid option was used in a socket option call."),
    E(EPROTONOSUPPORT, "The protocol is not implemented, or it has not been configured into the system."),
    E(ESOCKTNOSUPPORT, "The specified socket type does not exist in the address family."),
    E(EPFNOSUPPORT, "The protocol family is not implemented, or it has not been configured into the system."),
    E(EAFNOSUPPORT, "An address family incompatible with the requested protocol was used."),
    E(ENETDOWN, "The network is down."),
    E(ENETUNREACH, "The network is unreachable."),
    E(ENETRESET, "The operation failed because the subnet containing the remote host was unreachable."),
    E(ECONNABORTED, "The connection was aborted locally."),
    E(ECONNRESET, "The connection was forcibly closed by the remote host."),
    E(EISCONN, "A connect request was made on an already-connected socket."),
    E(ENOTCONN, "A send/receive operation was disallowed because the socket is not connected."),
    E(ESHUTDOWN, "A send/receive operation was disallowed because the socket is shut down."),
    E(EOPNOTSUPP, "The operation is not supported for this socket type."),
    E(ENOTSOCK, "An operation was attempted on an invalid socket file descriptor."),
    E(EBADFD, "An operation was attempted on an invalid file handle."),
    E(ETIMEDOUT, "The connection timed out because the remote host failed to respond."),
    E(ECONNREFUSED, "The connection was refused by the remote host - it may not be running a server."),
    E(EHOSTDOWN, "The remote host is down."),
    E(EHOSTUNREACH, "The remote host is unreachable."),
#ifdef _WIN32
    E(WSAEPROCLIM, "Too many processes are using Windows Sockets at this time."),
    E(WSASYSNOTREADY, "The network subsystem is unavailable."),
    E(WSAVERNOTSUPPORTED, "The requested Windows Sockets version is out of range."),
    E(WSANOTINITIALISED, "A successful WSAStartup has not been performed."),
    E(WSA_E_NO_MORE, "No more results can be returned from the lookup service."),
    E(WSA_E_CANCELLED, "The lookup service call was cancelled."),
    E(WSASERVICE_NOT_FOUND, "Service not found - either Bluetooth is not on, or there are no nearby devices."),
    E(WSA_INVALID_PARAMETER, "One or more parameters are invalid."),
    E(WSA_INVALID_HANDLE, "An internal event object handle is invalid."),
    E(WSA_NOT_ENOUGH_MEMORY, "Not enough memory to complete the operation."),
    E(WSATYPE_NOT_FOUND, "The specified class type was not found."),
    E(WSAHOST_NOT_FOUND, "The name or service is unknown."),
    E(WSATRY_AGAIN, "Temporary error in name resolution."),
    E(WSANO_RECOVERY, "Non-recoverable error in name resolution."),
#else
    E(EPERM, "Operation not permitted."),
    E(EBUSY, "The device or resource is currently in use."),
    E(EPIPE, "Broken pipe - the socket has been shut down and/or closed."),
    E(EPROTO, "The protocol encountered an unrecoverable error."),
    E(ENODEV, "No such device - check that you have a suitable hardware device and its driver."),
    E(ENXIO, "No such device or address - check that you have a suitable hardware device and its driver."),
    E(EAI_ADDRFAMILY, "The host does not have any network addresses in the requested address family."),
    E(EAI_AGAIN, "Temporary error in name resolution."),
    E(EAI_BADFLAGS, "Invalid value for 'ai_flags' field."),
    E(EAI_FAIL, "Non-recoverable error in name resolution."),
    E(EAI_FAMILY, "The requested address family is not supported, see EAFNOSUPPORT."),
    E(EAI_MEMORY, "No more memory available."),
    E(EAI_NODATA, "The host does not have any network addresses defined, see ENODATA."),
    E(EAI_NONAME, "The name and/or service is unknown."),
    E(EAI_SERVICE, "The service or port is not supported by the socket type."),
    E(EAI_SOCKTYPE, "The socket type is not supported."),
    E(EAI_SYSTEM, "A system error occurred."),
#endif
};
