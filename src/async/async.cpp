// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm> // std::max()
#include <thread>
#include <vector>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#else
#include <bit>
#include <mutex>

#include <liburing.h>
#endif

#include "async.hpp"
#include "sys/errcheck.hpp"

// The number of threads to start
// If the number of supported threads cannot be determined, 1 is created.
static const auto numThreads = std::max(std::thread::hardware_concurrency(), 1U);

// A vector of threads to serve as a thread pool
static std::vector<std::thread> workerThreadPool(numThreads);

// A completion key value to indicate a signaled interrupt
static constexpr int ASYNC_INTERRUPT = 1;

#ifdef _WIN32
// The IOCP handle
static HANDLE completionPort = nullptr;
#else
static std::mutex ringMutex;

static io_uring ring;

// Waits for an io_uring completion queue entry with a mutex lock.
int waitCQE(io_uring_cqe*& cqe, void*& userData) {
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
#endif

// Runs in each thread to handle completion results.
static void worker() {
#ifdef _WIN32
    DWORD numBytes;
    ULONG_PTR completionKey;
    LPOVERLAPPED overlapped = nullptr;

    while (true) {
        // Dequeue a completion packet from the system and check for the exit condition
        BOOL ret = GetQueuedCompletionStatus(completionPort, &numBytes, &completionKey, &overlapped, INFINITE);
        if (completionKey == ASYNC_INTERRUPT) break;

        // Get the structure with completion data, passed through the overlapped pointer
        // No locking is needed to modify the structure's fields - the calling coroutine will be suspended at this
        // point so mutually-exclusive access is guaranteed.
        if (!overlapped) continue;
        auto& result = *static_cast<Async::CompletionResult*>(overlapped);
        result.numBytes = numBytes;

        // Pass any failure back to the calling coroutine
        if (!ret) result.error = System::getLastError();

        // Resume the coroutine that started the operation
        if (result.coroHandle) result.coroHandle();
    }
#else
    while (true) {
        io_uring_cqe* cqe;
        void* userData = nullptr;

        // Wait for new completion queue entry (locked by mutex)
        int ret = waitCQE(cqe, userData);
        if (ret < 0) continue;
        if (!userData) continue;

        // Check for interrupt
        if (std::bit_cast<unsigned long long>(userData) == ASYNC_INTERRUPT) break;

        // Fill in completion result information
        auto& result = *static_cast<Async::CompletionResult*>(userData);
        if (cqe->res < 0) result.error = -cqe->res;
        else result.numBytes = cqe->res;

        // Resume calling coroutine
        if (result.coroHandle) result.coroHandle();
    }
#endif
}

void Async::init() {
#ifdef _WIN32
    // Initialize IOCP
    completionPort = EXPECT_TRUE(CreateIoCompletionPort, INVALID_HANDLE_VALUE, nullptr, 0, numThreads);
#else
    EXPECT_POSITIVE_RC(io_uring_queue_init, 128, &ring, 0);
#endif

    // Populate thread pool
    for (auto& i : workerThreadPool) i = std::thread{ worker };
}

void Async::cleanup() {
    // Signal threads to terminate
#ifdef _WIN32
    if (!completionPort) return;

    for ([[maybe_unused]] const auto& _ : workerThreadPool)
        PostQueuedCompletionStatus(completionPort, 0, ASYNC_INTERRUPT, nullptr);
#else
    for ([[maybe_unused]] const auto& _ : workerThreadPool) {
        // Submit a no-op to the queue to get the waiting call terminated
        io_uring_sqe* sqe = getUringSQE();
        io_uring_prep_nop(sqe);
        io_uring_sqe_set_data64(sqe, ASYNC_INTERRUPT);
    }
    submitRing();
#endif

    // Join threads
    // Manual join calls are used instead of a jthread so the program can wait (briefly) for all threads to exit
    // before cleaning up.
    for (auto& i : workerThreadPool) i.join();

#ifdef _WIN32
    CloseHandle(completionPort);
#else
    io_uring_queue_exit(&ring);
#endif
}

#ifdef _WIN32
void Async::add(SOCKET sockfd) {
    // Create a new handle for the socket
    EXPECT_TRUE(CreateIoCompletionPort, reinterpret_cast<HANDLE>(sockfd), completionPort, 0, 0);
}
#else
io_uring_sqe* Async::getUringSQE() { return io_uring_get_sqe(&ring); }

void Async::submitRing() { io_uring_submit(&ring); }
#endif
