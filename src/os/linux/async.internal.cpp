// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_LINUX
#include <vector>

#include <liburing.h>

#include "os/check.hpp"

module os.async.internal;
import os.async.platform.internal;
import os.async;
import os.errcheck;
import os.error;

void Async::Internal::init(unsigned int numThreads, unsigned int queueEntries) {
    rings.reserve(numThreads);

    for (unsigned int i = 0; i < numThreads; i++) {
        // Create new io_uring instance
        io_uring& ring = rings.emplace_back();

        // Initialize the instance
        CHECK(io_uring_queue_init(queueEntries, &ring, 0), checkZero, useReturnCodeNeg);
    }
}

void Async::Internal::stopThreads(unsigned int) {
    for (auto& ring : rings) {
        // Submit a no-op to the queue to get the waiting call terminated
        io_uring_sqe* sqe = io_uring_get_sqe(&ring);
        io_uring_prep_nop(sqe);
        io_uring_sqe_set_data64(sqe, ASYNC_INTERRUPT);
        io_uring_submit(&ring);
    }
}

void Async::Internal::cleanup() {
    for (auto& ring : rings) io_uring_queue_exit(&ring);
}

void Async::Internal::worker(unsigned int threadNum) {
    io_uring* ring = &rings[threadNum];

    while (true) {
        io_uring_cqe* cqe;

        // Wait for new completion queue entry
        if (io_uring_wait_cqe(ring, &cqe) != 0) continue;

        // Make sure the wait succeeded before getting user data pointer
        // When io_uring_wait_cqe() fails, cqe is null.
        // One such failure case is EINTR, when a breakpoint is hit in another part of the program while this is
        // waiting.
        void* userData = io_uring_cqe_get_data(cqe);
        io_uring_cqe_seen(ring, cqe);

        if (userData == nullptr) continue;

        // Check for interrupt
        if (reinterpret_cast<uint64_t>(userData) == ASYNC_INTERRUPT) break;

        // Fill in completion result information
        auto& result = *reinterpret_cast<CompletionResult*>(userData);
        if (cqe->res < 0) result.error = -cqe->res;
        else result.res = cqe->res;

        queueForCompletion(result);
    }
}
#endif
