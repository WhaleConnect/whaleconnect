// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <utility>

#if OS_WINDOWS
// Winsock headers
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <netdb.h> // addrinfo/getaddrinfo() related identifiers
#endif

#include "device.hpp"
#include "socket.hpp"

#include "utils/handleptr.hpp"
#include "utils/task.hpp"

// Winsock-specific definitions and their Berkeley equivalents
#if !OS_WINDOWS
constexpr auto GetAddrInfo = getaddrinfo;
constexpr auto FreeAddrInfo = freeaddrinfo;
using ADDRINFOW = addrinfo;
#endif

template <auto Tag>
struct ClientSocketTraits {};

template <auto Tag>
class ClientSocket : ClientSocketTraits<Tag>, public WritableSocket<Tag>, public Connectable {
    // TODO: Remove typename in Clang 16
    using typename SocketTraits<Tag>::HandleType;

    Device _device;

public:
    explicit ClientSocket(HandleType handle, Device device, ClientSocketTraits<Tag> traits = {}) :
        Socket<Tag>(handle), ClientSocketTraits<Tag>(std::move(traits)), _device(std::move(device)) {}

    ~ClientSocket() override = default;

    [[nodiscard]] const Device& getDevice() const { return _device; }

    [[nodiscard]] Task<> connect() const override;
};

// Creates a client socket managed by a unique_ptr.
template <auto Tag>
std::unique_ptr<ClientSocket<Tag>> createClientSocket(const Device& device);

template <>
struct ClientSocketTraits<SocketTag::IP> {
    HandlePtr<ADDRINFOW, FreeAddrInfo> _addr;
};
