// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "async.hpp"

#include <atomic>
#include <mutex>
#include <variant>
#include <vector>

#include <WinSock2.h>
#include <MSWSock.h>

#include "errcheck.hpp"
#include "net/enums.hpp"
#include "sockets/delegates/sockethandle.hpp"
#include "utils/overload.hpp"

// IOCP does not guarantee an event submitted on one thread will be resumed on that thread.
// Threads will often need to resubmit events to each other.
struct Resubmit {
    std::vector<std::coroutine_handle<>> handles;
    std::mutex m;
    std::atomic_bool hasHandles = false;
};

std::vector<Resubmit> resubmits;

HANDLE completionPort = nullptr; // IOCP handle
int runningThreads = 0;
std::mutex runningMutex;

LPFN_CONNECTEX connectExPtr = nullptr;
LPFN_ACCEPTEX acceptExPtr = nullptr;

void loadConnectEx(SOCKET s) {
    // Load the ConnectEx function
    GUID guid = WSAID_CONNECTEX;
    DWORD numBytes = 0;
    check(WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &connectExPtr, sizeof(connectExPtr),
        &numBytes, nullptr, nullptr));
}

void loadAcceptEx(SOCKET s) {
    // Load the AcceptEx function
    GUID guid = WSAID_ACCEPTEX;
    DWORD numBytes = 0;
    check(WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &acceptExPtr, sizeof(acceptExPtr),
        &numBytes, nullptr, nullptr));
}

void handleOperation(const Async::Operation& operation, std::size_t thread) {
    Overload visitor{
        [=](const Async::Connect& op) {
            check(connectExPtr(op.handle, op.addr, static_cast<int>(op.addrLen), nullptr, 0, nullptr, op.result),
                checkTrue);
        },
        [=](const Async::Accept& op) {
            constexpr DWORD addrSize = sizeof(sockaddr_storage) + 16;
            check(acceptExPtr(op.handle, op.clientSocket, op.buf, 0, addrSize, addrSize, nullptr, op.result),
                checkTrue);
        },
        [=](const Async::Send& op) {
            // Safe to use const_cast since WSASend/WSASendTo do not modify the buffer.
            WSABUF buf{ static_cast<ULONG>(op.data.size()), const_cast<char*>(op.data.data()) };
            check(WSASend(op.handle, &buf, 1, nullptr, 0, op.result, nullptr));
        },
        [=](const Async::SendTo& op) {
            WSABUF buf{ static_cast<ULONG>(op.data.size()), const_cast<char*>(op.data.data()) };
            check(WSASendTo(op.handle, &buf, 1, nullptr, 0, op.addr, static_cast<int>(op.addrLen), op.result, nullptr));
        },
        [=](const Async::Receive& op) {
            DWORD flags = 0;
            WSABUF buf{ static_cast<ULONG>(op.data.size()), op.data.data() };
            check(WSARecv(op.handle, &buf, 1, nullptr, &flags, op.result, nullptr));
        },
        [=](const Async::ReceiveFrom& op) {
            DWORD flags = 0;
            WSABUF buf{ static_cast<ULONG>(op.data.size()), op.data.data() };
            check(WSARecvFrom(op.handle, &buf, 1, nullptr, &flags, op.addr, op.fromLen, op.result, nullptr));
        },
        [=](const Async::Shutdown& op) { shutdown(op.handle, SD_BOTH); },
        [=](const Async::Close& op) { closesocket(op.handle); },
        [=](const Async::Cancel& op) { CancelIo(reinterpret_cast<HANDLE>(op.handle)); },
    };

    std::visit(
        [thread](const auto& op) {
            if (op.result) op.result->thread = thread;
        },
        operation);

    try {
        std::visit(visitor, operation);
    } catch (const System::SystemError& e) {
        Async::CompletionResult* result;
        std::visit([&](auto&& op) { result = op.result; }, operation);

        result->error = e.code;
        result->coroHandle();
    }
}

Async::EventLoop::EventLoop(unsigned int numThreads, unsigned int) {
    std::scoped_lock lock{ runningMutex };

    // Initialization and cleanup happen on the first thread that is initialized
    if (runningThreads == 0) {
        // Start Winsock
        WSADATA wsaData{};
        check(WSAStartup(MAKEWORD(2, 2), &wsaData), checkTrue, useReturnCode); // MAKEWORD(2, 2) for Winsock 2.2

        // Initialize IOCP
        completionPort = check(CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, numThreads), checkTrue);

        Delegates::SocketHandle<SocketTag::IP> tmp{ socket(AF_INET, SOCK_STREAM, 0) };
        loadConnectEx(*tmp);
        loadAcceptEx(*tmp);
    }

    thisId = runningThreads;
    runningThreads++;
    if (resubmits.empty()) {
        std::vector<Resubmit> tmp(numThreads);
        resubmits = std::move(tmp);
    }
}

Async::EventLoop::~EventLoop() {
    std::scoped_lock lock{ runningMutex };

    runningThreads--;
    if (runningThreads == 0) {
        // Cleanup Winsock
        CloseHandle(completionPort);
        check(WSACleanup());
    }
}

void Async::EventLoop::runOnce(bool wait) {
    // Check for submits from other threads
    Resubmit& pendingSubmits = resubmits[thisId];
    bool expected = true;
    if (pendingSubmits.hasHandles.compare_exchange_weak(expected, false, std::memory_order_relaxed)) {
        std::vector<std::coroutine_handle<>> tmp;
        {
            std::scoped_lock lock{ pendingSubmits.m };
            std::swap(tmp, pendingSubmits.handles);
        }

        numOperations -= tmp.size();
        for (auto i : tmp) i();
    }

    if (!operations.empty()) {
        for (const auto& i : operations) handleOperation(i, thisId);
        numOperations += operations.size();
        operations.clear();
    }

    DWORD numBytes;
    ULONG_PTR completionKey;
    LPOVERLAPPED overlapped = nullptr;

    // Dequeue a completion packet from the system and check for the exit condition
    // Shorter timeout than on other platforms - threads need to handle events that are not from IOCP.
    DWORD timeout = wait ? 10 : 0;
    BOOL ret = GetQueuedCompletionStatus(completionPort, &numBytes, &completionKey, &overlapped, timeout);

    // Get the structure with completion data, passed through the overlapped pointer
    // No locking is needed to modify the structure's fields - the calling coroutine will be suspended at this
    // point so mutually-exclusive access is guaranteed.
    if (!overlapped) return;

    auto& result = *static_cast<CompletionResult*>(overlapped);
    result.res = static_cast<int>(numBytes);

    // Pass any failure back to the calling coroutine
    if (!ret) result.error = System::getLastError();

    if (result.thread == thisId) {
        // This is the thread that started the operation
        numOperations--;
        result.coroHandle();
    } else {
        // Queue the event to the thread that started it
        Resubmit& r = resubmits[result.thread];
        std::scoped_lock lock{ r.m };
        r.handles.push_back(result.coroHandle);
        r.hasHandles.store(true, std::memory_order_relaxed);
    }
}

void Async::add(SOCKET s) {
    check(CreateIoCompletionPort(reinterpret_cast<HANDLE>(s), completionPort, 0, 0), checkTrue);
}
