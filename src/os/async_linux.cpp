// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_LINUX
#include "async_internal.hpp"

#include <bit>
#include <mutex>

#include <liburing.h>

#include "async.hpp"
#include "errcheck.hpp"

static std::mutex ringMutex;

static io_uring ring;

// Waits for an io_uring completion queue entry with a mutex lock.
static int waitCQE(io_uring_cqe*& cqe, void*& userData) {
    std::scoped_lock cqeLock{ ringMutex };
    int ret = io_uring_wait_cqe(&ring, &cqe);

    // Make sure the wait succeeded before getting user data pointer
    // When io_uring_wait_cqe() fails, cqe is null.
    // One such failure case is EINTR (ret == -4), when a breakpoint is hit in another part of the program while this is
    // waiting.
    if (ret == NO_ERROR) {
        userData = io_uring_cqe_get_data(cqe);
        io_uring_cqe_seen(&ring, cqe);
    }

    return ret;
}

bool Async::Internal::invalid() { return false; }

void Async::Internal::init() { EXPECT_POSITIVE_RC(io_uring_queue_init, 128, &ring, 0); }

void Async::Internal::stopThreads() {
    for (int i = 0; i < numThreads; i++) {
        // Submit a no-op to the queue to get the waiting call terminated
        io_uring_sqe* sqe = getUringSQE();
        io_uring_prep_nop(sqe);
        io_uring_sqe_set_data64(sqe, ASYNC_INTERRUPT);
    }
    submitRing();
}

void Async::Internal::cleanup() { io_uring_queue_exit(&ring); }

Async::Internal::WorkerResult Async::Internal::worker() {
    io_uring_cqe* cqe;
    void* userData = nullptr;

    // Wait for new completion queue entry (locked by mutex)
    int ret = waitCQE(cqe, userData);
    if (ret < 0) return resultError();
    if (!userData) return resultError();

    // Check for interrupt
    if (std::bit_cast<unsigned long long>(userData) == ASYNC_INTERRUPT) return resultInterrupt();

    // Fill in completion result information
    auto& result = toResult(userData);
    if (cqe->res < 0) result.error = -cqe->res;
    else result.res = cqe->res;

    return resultSuccess(result);
}

io_uring_sqe* Async::getUringSQE() { return io_uring_get_sqe(&ring); }

void Async::submitRing() { io_uring_submit(&ring); }
#endif
