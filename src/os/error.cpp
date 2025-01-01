// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <format>

#if OS_WINDOWS
#include <WinSock2.h>
#else
#include <cerrno>
#include <cstring>

#include <netdb.h>
#endif

#if OS_MACOS
#include <IOKit/IOReturn.h>
#include <mach/mach_error.h>
#endif

#include "error.hpp"

const char* getErrorName(System::ErrorType type) {
    using enum System::ErrorType;
    switch (type) {
        case System:
            return "System";
        case AddrInfo:
            return "getaddrinfo";
        case IOReturn:
            return "IOReturn";
        default:
            return "Unknown error type";
    }
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
    if (code == 0) return false;

#if OS_WINDOWS
    // Pending I/O for overlapped sockets
    if (code == WSA_IO_PENDING) return false;
#elif OS_MACOS
    // Pending I/O for non-blocking sockets
    if (code == EINPROGRESS) return false;
#endif

    return true;
}

std::string System::formatSystemError(ErrorCode code, ErrorType type, const std::source_location& location) {
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
#if OS_MACOS
        case IOReturn:
            msg = mach_error_string(code);
            break;
#endif
        default:
            msg = "Unknown error type";
    }

    std::string where = std::format("{}({}:{})", location.file_name(), location.line(), location.column());
    return std::format("{} (type {}, at {}): {}", code, getErrorName(type), where, msg);
}

bool System::SystemError::isCanceled() const {
#if OS_WINDOWS
    if (type == System::ErrorType::System && code == WSA_OPERATION_ABORTED) return true;
#else
    if (type == System::ErrorType::System && code == ECANCELED) return true;
#endif

#if OS_MACOS
    // IOBluetooth-specific error
    if (type == System::ErrorType::IOReturn && code == kIOReturnAborted) return true;
#endif

    return false;
}
