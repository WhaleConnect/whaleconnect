// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <format>

#if OS_WINDOWS
#include <WinSock2.h>
#else
#include <cerrno>  // errno
#include <cstring> // std::strerror()

#include <netdb.h> // gai_strerror()
#endif

#if OS_MACOS
#include <IOKit/IOReturn.h>
#include <mach/mach_error.h>
#endif

#include <magic_enum.hpp>
#include <magic_enum_format.hpp>

#include "error.hpp"

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name(System::ErrorType value) noexcept {
    if (value == System::ErrorType::AddrInfo) return "getaddrinfo";
    return default_tag;
}

System::ErrorCode System::getLastError() {
#if OS_WINDOWS
    return GetLastError();
#else
    return errno;
#endif
}

bool System::isFatal(ErrorCode code) {
    // Check if the code is actually an error
    // Platform-specific "pending" codes indicate an async operation has not yet finished and not a fatal error.
    if (code == NO_ERROR) return false;

#if OS_WINDOWS
    // Pending I/O for overlapped sockets
    if (code == WSA_IO_PENDING) return false;
#elif OS_MACOS
    // Pending I/O for non-blocking sockets
    if (code == EINPROGRESS) return false;
#endif

    return true;
}

std::string System::formatSystemError(ErrorCode code, ErrorType type, std::string_view fnName) {
    using enum ErrorType;

    // Message buffer
    std::string msg;

    switch (type) {
#if OS_WINDOWS
        case System:
        case AddrInfo: {
            // gai_strerror is not recommended on Windows:
            // https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getaddrinfo#return-value
            msg.resize(512);

            // Get the message text
            auto flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK;
            DWORD length = FormatMessageA(flags, nullptr, code, LocaleNameToLCID(L"en-US", 0), msg.data(),
                                          static_cast<DWORD>(msg.size()), nullptr);

            msg.resize(length);
            break;
        }
#else
        case System:
            msg = std::strerror(code);
            break;
        case AddrInfo:
            msg = gai_strerror(code);
            break;
#endif
        case IOReturn:
#if OS_MACOS
            msg = mach_error_string(code);
#else
            msg = "Unknown error type";
#endif
    }

    return std::format("{} (type {}, in {}): {}", code, type, fnName, msg);
}

bool System::SystemError::isCanceled() const {
#if OS_WINDOWS
    if ((type == System::ErrorType::System) && (code == WSA_OPERATION_ABORTED)) return true;
#else
    if ((type == System::ErrorType::System) && (code == ECANCELED)) return true;
#endif

#if OS_MACOS
    // IOBluetooth-specific error
    if ((type == System::ErrorType::IOReturn) && (code == kIOReturnAborted)) return true;
#endif

    return false;
}
