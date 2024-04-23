// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_WINDOWS
#include <WinSock2.h>
#elif OS_MACOS
#include <optional>

#include <BluetoothMacOS-Swift.h>
#endif

#include "net/enums.hpp"

namespace Traits {
    // Platform-specific traits for socket handles.
#if OS_WINDOWS
    template <auto Tag>
    struct SocketHandle {
        using HandleType = SOCKET;
        static constexpr auto invalidHandle = INVALID_SOCKET;
    };
#elif OS_MACOS
    template <auto Tag>
    struct SocketHandle {};

    template <>
    struct SocketHandle<SocketTag::IP> {
        using HandleType = int;
        static constexpr auto invalidHandle = -1;
    };

    template <>
    struct SocketHandle<SocketTag::BT> {
        using HandleType = std::optional<BluetoothMacOS::BTHandle>;
        static constexpr auto invalidHandle = std::nullopt;
    };
#elif OS_LINUX
    template <auto Tag>
    struct SocketHandle {
        using HandleType = int;
        static constexpr auto invalidHandle = -1;
    };
#endif

    // Convenience type alias for socket handle types.
    template <auto Tag>
    using SocketHandleType = SocketHandle<Tag>::HandleType;

    // Convenience function for invalid socket handle values.
    template <auto Tag>
    constexpr auto invalidSocketHandle() {
        return SocketHandle<Tag>::invalidHandle;
    }

    // Traits for server sockets.
    template <auto Tag>
    struct Server {};

    template <>
    struct Server<SocketTag::IP> {
        IPType ip;
    };
}
