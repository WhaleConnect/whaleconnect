// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <coroutine>
#include <string>
#include <string_view>
#include <utility> // std::exchange()

#if OS_APPLE
#include <variant>

#include <IOBluetooth/IOBluetoothUserLib.h>
#endif

#include "async.hpp"
#include "net.hpp"
#include "utils/task.hpp"

// A class to manage a socket file descriptor with RAII.
class Socket {
#if OS_APPLE
    // Wrapper structs for L2CAP/RFCOMM channels because IOBluetooth{L2CAP,RFCOMM}ChannelRef are aliased to the same
    // type and must be differentiated.
    struct L2CAPChannelWrapper {
        IOBluetoothL2CAPChannelRef channel;
    };

    struct RFCOMMChannelWrapper {
        IOBluetoothRFCOMMChannelRef channel;
    };

    using HandleType = std::variant<std::monostate, SOCKET, L2CAPChannelWrapper, RFCOMMChannelWrapper>;
    static constexpr auto _invalidHandle = std::monostate{};

    SOCKET _getFd() const { return std::get<SOCKET>(_handle); }
#else
    using HandleType = SOCKET;
    static constexpr auto _invalidHandle = INVALID_SOCKET;
#endif

    static constexpr size_t _recvLen = 1024;

    HandleType _handle;

    // Releases ownership of the managed handle.
    HandleType _release() { return std::exchange(_handle, _invalidHandle); }

public:
    // The result of a receive operation.
    struct RecvResult {
        size_t bytesRead = 0; // Number of bytes read
        std::string data;     // String received
    };

    // Constructs an object owning nothing.
    Socket() = default;

    // Constructs an object owning a file descriptor.
    explicit Socket(SOCKET fd) : _handle(fd) {}

    Socket(const Socket&) = delete;

    // Constructs an object, and transfers ownership from another object.
    Socket(Socket&& other) noexcept : _handle(other._release()) {}

    // Destructs this object and closes the managed handle.
    ~Socket() { close(); }

    Socket& operator=(const Socket&) = delete;

    // Transfers ownership from another object.
    Socket& operator=(Socket&& other) noexcept {
        close();
        _handle = other._release();

        return *this;
    }

    // Checks the validity of the managed socket.
    explicit operator bool() const;

    // Closes the managed socket.
    void close();

    // Sends a string through the socket.
    // The data is passed as a string (not a string_view) to make a copy and prevent dangling pointers in the coroutine.
    Task<> send(std::string data) const;

    // Receives a string from the socket.
    Task<RecvResult> recv() const;
};
