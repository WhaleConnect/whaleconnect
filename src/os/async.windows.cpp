// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "async.hpp"

#include <atomic>
#include <variant>

#include <WinSock2.h>
#include <MSWSock.h>

#include "errcheck.hpp"
#include "utils/overload.hpp"

HANDLE completionPort = nullptr; // IOCP handle
std::atomic_int runningThreads = 0;
std::atomic_size_t numOperations = 0;

LPFN_CONNECTEX loadConnectEx(SOCKET s) {
    static LPFN_CONNECTEX connectExPtr = nullptr;

    if (!connectExPtr) {
        // Load the ConnectEx function
        GUID guid = WSAID_CONNECTEX;
        DWORD numBytes = 0;
        check(WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &connectExPtr, sizeof(connectExPtr),
            &numBytes, nullptr, nullptr));
    }
    return connectExPtr;
}

LPFN_ACCEPTEX loadAcceptEx(SOCKET s) {
    static LPFN_ACCEPTEX acceptExPtr = nullptr;

    if (!acceptExPtr) {
        // Load the AcceptEx function
        GUID guid = WSAID_ACCEPTEX;
        DWORD numBytes = 0;
        check(WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &acceptExPtr, sizeof(acceptExPtr),
            &numBytes, nullptr, nullptr));
    }
    return acceptExPtr;
}

Async::EventLoop::EventLoop(unsigned int) {
    if (runningThreads.fetch_add(1, std::memory_order_relaxed) > 0) return;

    // Start Winsock
    WSADATA wsaData{};
    check(WSAStartup(MAKEWORD(2, 2), &wsaData), checkTrue, useReturnCode); // MAKEWORD(2, 2) for Winsock 2.2

    // Initialize IOCP
    completionPort = check(CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0), checkTrue);
}

Async::EventLoop::~EventLoop() {
    if (runningThreads.fetch_sub(1, std::memory_order_relaxed) > 1) return;

    // Cleanup Winsock
    CloseHandle(completionPort);
    check(WSACleanup());
}

void Async::EventLoop::push(const Operation& operation) {
    Overload visitor{
        [=](const Async::Connect& op) {
            auto connectExPtr = loadConnectEx(op.handle);
            check(connectExPtr(op.handle, op.addr, static_cast<int>(op.addrLen), nullptr, 0, nullptr, op.result),
                checkTrue);
            numOperations.fetch_add(1, std::memory_order_relaxed);
        },
        [=](const Async::Accept& op) {
            constexpr DWORD addrSize = sizeof(sockaddr_storage) + 16;
            auto acceptExPtr = loadAcceptEx(op.handle);
            check(acceptExPtr(op.handle, op.clientSocket, op.buf, 0, addrSize, addrSize, nullptr, op.result),
                checkTrue);
            numOperations.fetch_add(1, std::memory_order_relaxed);
        },
        [=](const Async::Send& op) {
            check(WSASend(op.handle, op.buf, 1, nullptr, 0, op.result, nullptr));
            numOperations.fetch_add(1, std::memory_order_relaxed);
        },
        [=](const Async::SendTo& op) {
            check(
                WSASendTo(op.handle, op.buf, 1, nullptr, 0, op.addr, static_cast<int>(op.addrLen), op.result, nullptr));
            numOperations.fetch_add(1, std::memory_order_relaxed);
        },
        [=](const Async::Receive& op) {
            DWORD flags = 0;
            check(WSARecv(op.handle, op.buf, 1, nullptr, &flags, op.result, nullptr));
            numOperations.fetch_add(1, std::memory_order_relaxed);
        },
        [=](const Async::ReceiveFrom& op) {
            DWORD flags = 0;
            check(WSARecvFrom(op.handle, op.buf, 1, nullptr, &flags, op.addr, op.fromLen, op.result, nullptr));
            numOperations.fetch_add(1, std::memory_order_relaxed);
        },
        [=](const Async::Shutdown& op) { shutdown(op.handle, SD_BOTH); },
        [=](const Async::Close& op) { closesocket(op.handle); },
        [=](const Async::Cancel& op) { CancelIoEx(reinterpret_cast<HANDLE>(op.handle), nullptr); },
    };

    std::visit(visitor, operation);
}

void Async::EventLoop::runOnce(bool wait) {
    if (numOperations.load(std::memory_order_relaxed) == 0) return;

    DWORD numBytes;
    ULONG_PTR completionKey;
    LPOVERLAPPED overlapped = nullptr;

    // Dequeue a completion packet from the system and check for the exit condition
    DWORD timeout = wait ? 200 : 0;
    BOOL ret = GetQueuedCompletionStatus(completionPort, &numBytes, &completionKey, &overlapped, timeout);

    // Get the structure with completion data, passed through the overlapped pointer
    // No locking is needed to modify the structure's fields - the calling coroutine will be suspended at this
    // point so mutually-exclusive access is guaranteed.
    if (!overlapped) return;
    numOperations.fetch_sub(1, std::memory_order_relaxed);

    auto& result = *static_cast<CompletionResult*>(overlapped);
    result.res = static_cast<int>(numBytes);

    // Pass any failure back to the calling coroutine
    if (!ret) result.error = System::getLastError();

    result.coroHandle();
}

void Async::add(SOCKET s) {
    check(CreateIoCompletionPort(reinterpret_cast<HANDLE>(s), completionPort, 0, 0), checkTrue);
}
