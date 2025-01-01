// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <coroutine>
#include <functional>
#include <thread>
#include <variant>
#include <vector>

#if OS_WINDOWS
#include <WinSock2.h>
#include <WS2tcpip.h>
#elif OS_MACOS
#include <unordered_map>

#include <sys/event.h>
#include <unistd.h>
#elif OS_LINUX
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
        std::size_t thread = 0;

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
#if !OS_MACOS
        std::string_view data;
#endif
    };

    struct SendTo : OperationBase {
#if !OS_MACOS
        std::string_view data;
        sockaddr* addr;
        socklen_t addrLen;
#endif
    };

    struct Receive : OperationBase {
#if !OS_MACOS
        std::string& data;
#endif
    };

    struct ReceiveFrom : OperationBase {
#if OS_WINDOWS
        std::string& data;
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
        std::size_t thisId;
#elif OS_MACOS
        int kq = -1;
        PendingEventsMap pendingEvents;
#elif OS_LINUX
        io_uring ring;
#endif

        std::vector<Operation> operations;
        std::size_t numOperations = 0; // Events that are being waited on (not events in the queue)

    public:
        EventLoop(unsigned int numThreads, unsigned int queueEntries);

        ~EventLoop();

        // Runs one iteration of this event loop.
        void runOnce(bool wait = true);

        // Returns the number of I/O events that are being waited on.
        std::size_t size() {
            return numOperations;
        }

        void push(const Operation& operation) {
            operations.push_back(operation);
        }
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

    // Initializes the OS async APIs.
    // Returns the total number of threads created, including the main thread.
    unsigned int init(unsigned int numThreads, unsigned int queueEntries);

    // Explicit cleanup is needed for guaranteed object destruction order.
    void cleanup();

    // Submits an I/O operation to the async event loop.
    void submit(const Operation& op);

    // Submits work to a worker thread.
    Task<> queueToThread();

    // Extended queueToThread that can be used to queue to a specific thread.
    // If id == std::thread::id{} then the function is queued to all threads.
    // If the function returns true, it is re-queued onto the thread.
    void queueToThreadEx(std::thread::id id, std::function<Task<bool>()> f);

    // Runs one iteration of the main thread's event loop with an optional timeout.
    void handleEvents(bool wait = true);

#if OS_WINDOWS
    // Adds a socket to IOCP.
    void add(SOCKET s);
#elif OS_MACOS
    // Makes a socket nonblocking for use with kqueue.
    void prepSocket(int s);
#endif
}
