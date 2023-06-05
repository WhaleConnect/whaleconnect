// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include "async_macos.hpp"

#include "os/async_internal.hpp"

#include <array>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <unordered_map>

#include <IOKit/IOReturn.h>
#include <magic_enum.hpp>
#include <sys/event.h>

#include "os/async.hpp"
#include "os/errcheck.hpp"

static constexpr auto ASYNC_CANCEL = 2;

static int kq = -1;

// Map to track pending socket operations with their coroutines
// The kqueue system does not provide a way to cancel pending events, so this must be kept track of manually.
static std::unordered_map<int, Async::CompletionResult*> pendingSockets;
static std::mutex mapMutex;

struct BluetoothQueue {
    std::array<Async::CompletionResult*, 2> pending;  // Pending I/O operations (read/write stored separately)
    std::queue<std::optional<std::string>> completed; // Completed read operations
};

static std::unordered_map<uint64_t, BluetoothQueue> bluetoothChannels;

void Async::Internal::init(unsigned int) {
    kq = call(FN(kqueue));
}

void Async::Internal::stopThreads(unsigned int) {
    // Submit a single event to wake up all threads
    struct kevent event {};

    EV_SET(&event, ASYNC_INTERRUPT, EVFILT_USER, EV_ADD, NOTE_TRIGGER, 0, nullptr);
    kevent(kq, &event, 1, nullptr, 0, nullptr);
}

void Async::Internal::cleanup() {
    // Delete the user event
    struct kevent event {};

    EV_SET(&event, ASYNC_INTERRUPT, EVFILT_USER, EV_DELETE, 0, 0, nullptr);
    kevent(kq, &event, 1, nullptr, 0, nullptr);
}

Async::CompletionResult Async::Internal::worker() {
    // Wait for new event
    struct kevent event {};

    call(FN(kevent, kq, nullptr, 0, &event, 1, nullptr));

    // Check for interrupt
    if (event.filter == EVFILT_USER) {
        if (event.ident == ASYNC_INTERRUPT) {
            throw WorkerInterruptedError{};
        } else if (event.ident == ASYNC_CANCEL) {
            // Socket file descriptor associated with cancellation
            auto fd = static_cast<int>(event.data);

            std::scoped_lock lock{ mapMutex };
            if (!pendingSockets.contains(fd)) throw WorkerNoDataError{};

            // Completion result pointing to suspended coroutine
            auto& result = *pendingSockets[fd];
            pendingSockets.erase(fd);

            // Delete the socket's event from the kqueue
            struct kevent eventDelete;
            EV_SET(&eventDelete, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
            call(FN(kevent, kq, &eventDelete, 1, nullptr, 0, nullptr));

            // Set error and return coroutine handle
            result.error = ECANCELED;
            return result;
        }
    }

    auto& result = toResult(event.udata);

    // Set error and result
    if (event.flags & EV_EOF) result.error = static_cast<System::ErrorCode>(event.fflags);
    else result.res = static_cast<int>(event.data);

    // Remove the file descriptor from the pending map
    std::scoped_lock lock{ mapMutex };
    pendingSockets.erase(static_cast<int>(event.ident));

    return result;
}

void Async::submitKqueue(int ident, int filter, CompletionResult& result) {
    // The EV_ONESHOT flag will delete the event once it is retrieved in one of the threads.
    // This ensures only one thread will wake up to handle the event.
    struct kevent event {};

    EV_SET(&event, ident, filter, EV_ADD | EV_ONESHOT, 0, 0, &result);
    call(FN(kevent, kq, &event, 1, nullptr, 0, nullptr));

    std::scoped_lock lock{ mapMutex };
    pendingSockets[ident] = &result;
}

void Async::cancelPending(int fd) {
    struct kevent event {};

    EV_SET(&event, ASYNC_CANCEL, EVFILT_USER, EV_ADD | EV_ONESHOT, NOTE_TRIGGER, fd, nullptr);
    call(FN(kevent, kq, &event, 1, nullptr, 0, nullptr));
}

void Async::submitIOBluetooth(uint64_t id, BluetoothIOType type, CompletionResult& result) {
    // TODO: Use std::to_underlying() in C++23
    int idx = magic_enum::enum_integer(type);

    bluetoothChannels[id].pending[idx] = &result;
}

void Async::bluetoothComplete(uint64_t id, BluetoothIOType type, IOReturn status) {
    int idx = magic_enum::enum_integer(type);
    auto completionResult = bluetoothChannels[id].pending[idx];

    if (!completionResult) return;
    bluetoothChannels[id].pending[idx] = nullptr;

    completionResult->error = status;
    completionResult->coroHandle();
}

void Async::bluetoothReadComplete(uint64_t id, const char* data, size_t dataLen) {
    bluetoothChannels[id].completed.emplace(std::in_place, data, dataLen);

    bluetoothComplete(id, BluetoothIOType::Receive, kIOReturnSuccess);
}

void Async::bluetoothClosed(uint64_t id) {
    auto& completedQueue = bluetoothChannels[id].completed;
    completedQueue = {};

    completedQueue.emplace();
}

std::optional<std::string> Async::getBluetoothReadResult(uint64_t id) {
    auto data = bluetoothChannels[id].completed.front();
    bluetoothChannels[id].completed.pop();
    return data;
}
#endif
