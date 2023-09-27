// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_LINUX
#include <vector>

#include <liburing.h>

module os.async.platform;
import os.async.platform.internal;

int currentRingIdx = 0;

io_uring_sqe* Async::getUringSQE() {
    return io_uring_get_sqe(&Internal::rings[currentRingIdx]);
}

void Async::submitRing() {
    io_uring_submit(&Internal::rings[currentRingIdx]);

    currentRingIdx = (currentRingIdx + 1) % Internal::rings.size();
}

void Async::cancelPending(int fd) {
    for (auto& ring : Internal::rings) {
        io_uring_sqe* sqe = io_uring_get_sqe(&ring);
        io_uring_prep_cancel_fd(sqe, fd, IORING_ASYNC_CANCEL_ALL);
        io_uring_submit(&ring);
    }
}
#endif
