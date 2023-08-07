// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_LINUX
#include <liburing.h>

#include "os/async.hpp"

namespace Async {
    // Gets a submission queue entry from io_uring.
    io_uring_sqe* getUringSQE();

    // Submits entries into the io_uring submission queue.
    void submitRing();

    // Cancels pending operations on a socket.
    void cancelPending(int fd);
}
#endif
