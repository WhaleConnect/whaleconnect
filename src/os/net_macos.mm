// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_APPLE
#include "net_internal.hpp"

#include <fcntl.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h> // close()

#include "async.hpp"
#include "errcheck.hpp"
#include "net.hpp"

void Net::init() {}

void Net::cleanup() {}

void Net::Internal::startConnect(SOCKET s, const sockaddr* addr, socklen_t len, bool, Async::CompletionResult& result) {
    // Make socket non-blocking
    int flags = EXPECT_NONERROR(fcntl, s, F_GETFL, 0);
    EXPECT_NONERROR(fcntl, s, F_SETFL, flags | O_NONBLOCK);

    // Start connect
    EXPECT_ZERO(connect, s, addr, len);
    Async::submitKqueue(s, EVFILT_WRITE, result);
}

void Net::Internal::finalizeConnect(SOCKET, bool) {}

Task<Socket> Net::Internal::createClientSocketBT(const DeviceData& data) { co_return {}; }
#endif
