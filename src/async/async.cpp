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

static const auto numThreads = std::max(std::thread::hardware_concurrency(), 1U);

static std::vector<std::thread> workerThreadPool(numThreads);

#ifdef _WIN32
static constexpr int iocpInterrupt = 1;

static HANDLE completionPort = nullptr;

static void worker() {
    DWORD numBytes;
    uint64_t completionKey;
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
    completionPort = CALL_EXPECT_TRUE(CreateIoCompletionPort, INVALID_HANDLE_VALUE, nullptr, 0, numThreads);

    for (auto& i : workerThreadPool) i = std::thread(worker);
}

void Async::cleanup() {
    if (!completionPort) return;

    for (size_t i = 0; i < workerThreadPool.size(); i++)
        PostQueuedCompletionStatus(completionPort, 0, iocpInterrupt, nullptr);

    for (auto& i : workerThreadPool) if (i.joinable()) i.join();

    CloseHandle(completionPort);
}

void Async::add(SOCKET sockfd) {
#ifdef _WIN32
    CALL_EXPECT_TRUE(CreateIoCompletionPort, reinterpret_cast<HANDLE>(sockfd), completionPort, 0, 0);
#endif
}
