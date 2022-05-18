// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <thread>
#include <algorithm> // std::max()

#ifdef _WIN32
#include <unordered_map>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Mswsock.h>
#endif

#include "async.hpp"
#include "sys/errcheck.hpp"

// The number of threads to start
// If the number of supported threads cannot be determined, 1 is created.
static const auto numThreads = std::max(std::thread::hardware_concurrency(), 1U);

// A vector of threads to serve as a thread pool
static std::vector<std::thread> workerThreadPool(numThreads);

#ifdef _WIN32
// A completion key value to indicate a signaled interrupt
static constexpr int iocpInterrupt = 1;

// The IOCP handle
static HANDLE completionPort = nullptr;

// Runs in each thread to handle completion results.
static void worker() {
    DWORD numBytes;
    ULONG_PTR completionKey;
    LPOVERLAPPED overlapped = nullptr;

    while (true) {
        // Dequeue a completion packet from the system and check for the exit condition
        BOOL ret = GetQueuedCompletionStatus(completionPort, &numBytes, &completionKey, &overlapped, INFINITE);
        if (completionKey == iocpInterrupt) break;

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
}
#endif

void Async::init() {
#ifdef _WIN32
    // Initialize IOCP
    completionPort = CALL_EXPECT_TRUE(CreateIoCompletionPort, INVALID_HANDLE_VALUE, nullptr, 0, numThreads);
#endif

    // Populate thread pool
    for (auto& i : workerThreadPool) i = std::thread{ worker };
}

void Async::cleanup() {
    if (!completionPort) return;

#ifdef _WIN32
    // Signal threads to terminate
    for (size_t i = 0; i < workerThreadPool.size(); i++)
        PostQueuedCompletionStatus(completionPort, 0, iocpInterrupt, nullptr);

    // Join each thread
    for (auto& i : workerThreadPool) if (i.joinable()) i.join();

    // Close the IOCP handle
    CloseHandle(completionPort);
#endif
}

void Async::add(SOCKET sockfd) {
#ifdef _WIN32
    // Create a new handle for the socket
    CALL_EXPECT_TRUE(CreateIoCompletionPort, reinterpret_cast<HANDLE>(sockfd), completionPort, 0, 0);
#endif
}
