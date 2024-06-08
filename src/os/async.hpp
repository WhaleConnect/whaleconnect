// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <coroutine>
#include <variant>

#if OS_WINDOWS
#include <atomic>

#include <WinSock2.h>
#include <WS2tcpip.h>
#elif OS_MACOS
#include <unordered_map>
#include <vector>

#include <sys/event.h>
#include <unistd.h>
#elif OS_LINUX
#include <vector>

#include <liburing.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "error.hpp"
#include "net/enums.hpp"
#include "sockets/delegates/traits.hpp"
#include "utils/task.hpp"

namespace Async {
    // The information needed to resume a completion operation.
    //
    // This structure contains functions to make it an awaitable type. Calling co_await on an instance stores the
    // current coroutine handle to allow it to be resumed when the asynchronous operation finishes.
    struct CompletionResult
#if OS_WINDOWS
        // Inherit from OVERLAPPED on Windows to pass this struct as IOCP user data
        // https://devblogs.microsoft.com/oldnewthing/20101217-00/?p=11983
        : OVERLAPPED
#endif
    {
        std::coroutine_handle<> coroHandle; // The handle to the coroutine that started the operation
        System::ErrorCode error = 0; // The return code of the asynchronous function (returned to caller)
        int res = 0; // The result the operation (returned to caller, exact meaning depends on operation)

#if OS_WINDOWS
        CompletionResult() : OVERLAPPED{} {}
#else
        CompletionResult() = default;
#endif

        // Throws an exception if a fatal error occurred asynchronously.
        void checkError(System::ErrorType type) const {
            if (System::isFatal(error)) throw System::SystemError{ error, type };
        }

        // Checks if coroutine suspension is necessary.
        [[nodiscard]] bool await_ready() const noexcept {
            return static_cast<bool>(coroHandle);
        }

        // Stores the current coroutine handle to be resumed on completion.
        bool await_suspend(std::coroutine_handle<> coroutine) noexcept {
            coroHandle = coroutine;
            return false;
        }

        // Does nothing (nothing to do on coroutine resume).
        void await_resume() const noexcept {}
    };

    struct OperationBase {
        Traits::SocketHandleType<SocketTag::IP> handle;
        CompletionResult* result;
    };

    struct Connect : OperationBase {
#if !OS_MACOS
        sockaddr* addr;
        socklen_t addrLen;
#endif
    };

    struct Accept : OperationBase {
#if OS_WINDOWS
        SOCKET clientSocket;
        BYTE* buf;
#elif OS_LINUX
        sockaddr* addr;
        socklen_t* addrLen;
#endif
    };

    struct Send : OperationBase {
#if OS_WINDOWS
        WSABUF* buf;
#elif OS_LINUX
        std::string_view data;
#endif
    };

    struct SendTo : OperationBase {
#if OS_WINDOWS
        WSABUF* buf;
        sockaddr* addr;
        socklen_t addrLen;
#elif OS_LINUX
        std::string_view data;
        sockaddr* addr;
        socklen_t addrLen;
#endif
    };

    struct Receive : OperationBase {
#if OS_WINDOWS
        WSABUF* buf;
#elif OS_LINUX
        std::string& data;
#endif
    };

    struct ReceiveFrom : OperationBase {
#if OS_WINDOWS
        WSABUF* buf;
        sockaddr* addr;
        socklen_t* fromLen;
#elif OS_LINUX
        msghdr* msg;
#endif
    };

    struct Shutdown : OperationBase {};

    struct Close : OperationBase {};

    struct Cancel : OperationBase {};

    using Operation = std::variant<Connect, Accept, Send, SendTo, Receive, ReceiveFrom, Shutdown, Close, Cancel>;

#if OS_MACOS
    using PendingEventsMap = std::unordered_map<std::uint64_t, Async::CompletionResult*>;
#endif

    class EventLoop {
#if OS_WINDOWS
        inline static HANDLE completionPort = nullptr; // IOCP handle
        inline static int runningThreads = 0;
        inline static std::atomic_size_t numOperations = 0;
#elif OS_MACOS
        int kq = -1;
        PendingEventsMap pendingEvents;
#elif OS_LINUX
        io_uring ring;
#endif

#if !OS_WINDOWS
        std::vector<Operation> operations;
        std::size_t numOperations = 0;
#endif

    public:
        EventLoop(unsigned int queueEntries);

        ~EventLoop();

        void runOnce(bool wait = true);

#if OS_WINDOWS
        static void push(const Operation& operation);

        static void add(SOCKET s);
#else
        void push(const Operation& operation) {
            operations.push_back(operation);
        }
#endif
    };

    // Awaits an asynchronous operation and returns the result.
    Task<CompletionResult> run(auto fn, System::ErrorType type = System::ErrorType::System) {
        CompletionResult result;
        co_await result;

        fn(result);

        co_await std::suspend_always{};
        result.checkError(type);

        co_return result;
    }

    void init(unsigned int numThreads, unsigned int queueEntries);

    void submit(const Operation& op);

    Task<> queueToThread();

    void handleEvents(bool wait = true);

#if OS_WINDOWS
    inline void add(SOCKET s) {
        EventLoop::add(s);
    }
#elif OS_MACOS
    // Makes a socket nonblocking for use with kqueue.
    void prepSocket(int s);
#endif
}
