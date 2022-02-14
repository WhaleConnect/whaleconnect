// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#include <WinSock2.h>
#else
#include <cerrno> // errno
#endif

#include "error.hpp"
#include "util/formatcompat.hpp"

System::ErrorCode System::getLastErr() {
#ifdef _WIN32
    return GetLastError();
#else
    return errno;
#endif
}

void System::setLastErr(ErrorCode code) {
#ifdef _WIN32
    SetLastError(code);
#else
    errno = code;
#endif
}

std::string System::formatErr(ErrorCode code) {
    const char* formatStr = "{}: {}";

#ifdef _WIN32
    // Message buffer
    char msg[512];

    // Get the message text
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                   nullptr,
                   code,
                   LocaleNameToLCID(L"en-US", 0),
                   msg,
                   static_cast<DWORD>(std::ssize(msg)),
                   nullptr);

    return std::format(formatStr, code, msg);
#else
    // `strerrordesc_np()` (a GNU extension) is used since it doesn't translate the error message. A translation is
    // undesirable since the rest of the program isn't translated either.
    // A negative error value on Linux is most likely a getaddrinfo() error, handle that as well.
    return std::format(formatStr, code, (code >= 0) ? strerrordesc_np(code) : gai_strerror(code));
#endif
}

std::string System::formatLastErr() {
    return formatErr(getLastErr());
}

template <class T>
bool System::MayFail<T>::_nonFatal() {
    // Check if the code is actually an error
    if (_errCode == NO_ERROR) return false;

#ifdef _WIN32
    // This error means an operation hasn't failed, it's still waiting.
    // Tell the calling function that there's no error, and it should check back later.
    if (_errCode == WSA_IO_PENDING) return false;
#endif

    // The error is fatal
    return true;
}
