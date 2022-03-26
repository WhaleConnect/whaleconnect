// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <thread>
#include <algorithm> // std::max()

#ifdef _WIN32
#include <unordered_map>

#define NOMINMAX

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Mswsock.h>
#endif

#include "async.hpp"

static const auto numThreads = std::max(std::thread::hardware_concurrency(), 1U);

static std::vector<std::thread> workerThreadPool(numThreads);

#ifdef _WIN32
static HANDLE completionPort = nullptr;

static void worker() {
    DWORD numBytes;
    uint64_t completionKey;
    LPOVERLAPPED overlapped = nullptr;

    while (GetQueuedCompletionStatus(completionPort, &numBytes, &completionKey, &overlapped, INFINITE)) {
        if (!overlapped) break;

        auto& result = *reinterpret_cast<Async::CompletionResult*>(overlapped);
        result.numBytes = numBytes;
        if (result.coroHandle) result.coroHandle();
    }
}
#endif

System::MayFail<> Async::init() {
    completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, numThreads);
    if (!completionPort) return false;

    try {
        for (auto& i : workerThreadPool) i = std::thread(worker);
    } catch (const std::system_error&) {
        System::setLastErr(ERROR_NO_SYSTEM_RESOURCES);
        return false;
    }

    return true;
}

void Async::cleanup() {
    if (!completionPort) return;

    for (size_t i = 0; i < workerThreadPool.size(); i++) PostQueuedCompletionStatus(completionPort, 0, 0, nullptr);

    for (auto& i : workerThreadPool) if (i.joinable()) i.join();

    CloseHandle(completionPort);
}

System::MayFail<> Async::add(SOCKET sockfd) {
#ifdef _WIN32
    return static_cast<bool>(CreateIoCompletionPort(reinterpret_cast<HANDLE>(sockfd), completionPort, 0, 0));
#endif
}
