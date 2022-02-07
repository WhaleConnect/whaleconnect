// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <thread>
#include <algorithm> // std::max()

#ifdef _WIN32
#include <unordered_map>

#define NOMINMAX

#include <winsock2.h>
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

        auto& asyncData = *reinterpret_cast<Async::AsyncData*>(overlapped);
    }
}
#endif

int Async::init() {
    completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, numThreads);
    if (!completionPort) return SOCKET_ERROR;

    try {
        for (auto& i : workerThreadPool) i = std::thread(worker);
    } catch (const std::system_error&) {
        System::setLastErr(ERROR_NO_SYSTEM_RESOURCES);
        return SOCKET_ERROR;
    }

    return NO_ERROR;
}

void Async::cleanup() {
    if (!completionPort) return;

    for (size_t i = 0; i < workerThreadPool.size(); i++) PostQueuedCompletionStatus(completionPort, 0, 0, nullptr);

    for (auto& i : workerThreadPool) if (i.joinable()) i.join();

    CloseHandle(completionPort);
}

bool Async::add(SOCKET sockfd) {
#ifdef _WIN32
    return static_cast<bool>(CreateIoCompletionPort(reinterpret_cast<HANDLE>(sockfd), completionPort, 0, 0));
#endif
}

#ifdef _WIN32
HANDLE Async::getCompletionPort() {
    return completionPort;
}
#endif
