// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_WINDOWS
// Winsock header
#include <WinSock2.h>
#elif OS_MACOS
#include "net/bthandle.h"
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
#if __OBJC__
        using HandleType = BTHandle*;
#else
        // Objective-C types are not visible to C++ code, use a type of the same size as a filler
        // Class pointers take up 8 bytes: https://en.wikipedia.org/wiki/Tagged_pointer#Examples
        // On 64-bit systems, C++ pointers also take up 8 bytes
        using HandleType = int*;
#endif

        static constexpr auto invalidHandle = nullptr;
    };
#else
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
}
