// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <format>

#if OS_WINDOWS
#include <WinSock2.h>
#else
#include <cerrno>  // errno
#include <cstring> // std::strerror()

#include <netdb.h> // gai_strerror()
#endif

#include <magic_enum.hpp>

#include "error.hpp"

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name(System::ErrorType value) noexcept {
    if (value == System::ErrorType::AddrInfo) return "getaddrinfo";
    return default_tag;
}

std::string System::SystemError::formatted() const {
#if OS_WINDOWS
    // Message buffer
    std::string msg(512, '\0');

    // Get the message text
    auto formatFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK;
    DWORD length = FormatMessageA(formatFlags, nullptr, code, LocaleNameToLCID(L"en-US", 0), msg.data(),
                                  static_cast<DWORD>(msg.size()), nullptr);

    msg.resize(length);

#else
    auto msg = (type == ErrorType::System) ? std::strerror(code) : gai_strerror(code);
#endif

    return std::format("{} (type {}, in {}): {}", code, magic_enum::enum_name(type), fnName, msg);
}

System::ErrorCode System::getLastError() {
#if OS_WINDOWS
    return GetLastError();
#else
    return errno;
#endif
}

void System::setLastError(ErrorCode code) {
#if OS_WINDOWS
    SetLastError(code);
#else
    errno = code;
#endif
}

bool System::isFatal(ErrorCode code) {
    // Check if the code is actually an error
    if (code == NO_ERROR) return false;

#if OS_WINDOWS
    // This error means an operation hasn't failed, it's still waiting.
    // Tell the calling function that there's no error, and it should check back later.
    else if (code == WSA_IO_PENDING) return false;
#endif

    return true;
}
