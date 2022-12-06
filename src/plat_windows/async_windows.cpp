// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include "async_internal.hpp"

#include <WinSock2.h>

#include "async.hpp"
#include "errcheck.hpp"

// The IOCP handle
static HANDLE completionPort = nullptr;

bool Async::Internal::invalid() { return completionPort == nullptr; }

void Async::Internal::init() {
    // Start Winsock
    WSADATA wsaData{};
    EXPECT_ZERO_RC(WSAStartup, MAKEWORD(2, 2), &wsaData); // MAKEWORD(2, 2) for Winsock 2.2

    // Initialize IOCP
    completionPort = EXPECT_TRUE(CreateIoCompletionPort, INVALID_HANDLE_VALUE, nullptr, 0, numThreads);
}

void Async::Internal::stopThreads() {
    for (size_t i = 0; i < numThreads; i++) PostQueuedCompletionStatus(completionPort, 0, ASYNC_INTERRUPT, nullptr);
}

void Async::Internal::cleanup() {
    CloseHandle(completionPort);

    // Cleanup Winsock
    EXPECT_ZERO(WSACleanup);
}

Async::Internal::WorkerResult Async::Internal::worker() {
    DWORD numBytes;
    ULONG_PTR completionKey;
    LPOVERLAPPED overlapped = nullptr;

    // Dequeue a completion packet from the system and check for the exit condition
    BOOL ret = GetQueuedCompletionStatus(completionPort, &numBytes, &completionKey, &overlapped, INFINITE);
    if (completionKey == ASYNC_INTERRUPT) return resultInterrupted();

    // Get the structure with completion data, passed through the overlapped pointer
    // No locking is needed to modify the structure's fields - the calling coroutine will be suspended at this
    // point so mutually-exclusive access is guaranteed.
    if (!overlapped) return resultError();
    auto& result = toResult(overlapped);
    result.res = static_cast<int>(numBytes);

    // Pass any failure back to the calling coroutine
    if (!ret) result.error = System::getLastError();

    return resultSuccess(result);
}

void Async::add(SOCKET sockfd) {
    // Create a new handle for the socket
    EXPECT_TRUE(CreateIoCompletionPort, reinterpret_cast<HANDLE>(sockfd), completionPort, 0, 0);
}
#endif
