// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_WINDOWS
#include <WinSock2.h>

#include "os/check.hpp"

module os.async.internal;
import os.async.platform.internal;
import os.async;
import os.errcheck;

void Async::Internal::init(unsigned int numThreads, unsigned int) {
    // Start Winsock
    WSADATA wsaData{};
    CHECK(WSAStartup(MAKEWORD(2, 2), &wsaData), checkTrue, useReturnCode); // MAKEWORD(2, 2) for Winsock 2.2

    // Initialize IOCP
    completionPort = CHECK(CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, numThreads), checkTrue);
}

void Async::Internal::stopThreads(unsigned int numThreads) {
    for (size_t i = 0; i < numThreads; i++) PostQueuedCompletionStatus(completionPort, 0, ASYNC_INTERRUPT, nullptr);
}

void Async::Internal::cleanup() {
    CloseHandle(completionPort);

    // Cleanup Winsock
    CHECK(WSACleanup());
}

void Async::Internal::worker(unsigned int) {
    while (true) {
        DWORD numBytes;
        ULONG_PTR completionKey;
        LPOVERLAPPED overlapped = nullptr;

        // Dequeue a completion packet from the system and check for the exit condition
        BOOL ret = GetQueuedCompletionStatus(completionPort, &numBytes, &completionKey, &overlapped, INFINITE);
        if (completionKey == ASYNC_INTERRUPT) break;

        // Get the structure with completion data, passed through the overlapped pointer
        // No locking is needed to modify the structure's fields - the calling coroutine will be suspended at this
        // point so mutually-exclusive access is guaranteed.
        if (overlapped == nullptr) continue;

        auto& result = *static_cast<CompletionResult*>(overlapped);
        result.res = static_cast<int>(numBytes);

        // Pass any failure back to the calling coroutine
        if (!ret) result.error = System::getLastError();

        queueForCompletion(result);
    }
}
#endif
