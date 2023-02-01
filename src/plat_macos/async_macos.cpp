// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include "os/async_internal.hpp"

#include <mutex>
#include <stdexcept>
#include <unordered_map>

#include <sys/event.h>

#include "os/async.hpp"
#include "os/errcheck.hpp"

static constexpr auto ASYNC_CANCEL = 2;

static int kq = -1;

// Map to track pending socket operations with their coroutines
// The kqueue system does not provide a way to cancel pending events, so this must be kept track of manually.
static std::unordered_map<int, Async::CompletionResult*> pendingSockets;
static std::mutex mapMutex;

static std::unordered_map<IOBluetoothObjectID, Async::CompletionResult*> pendingBluetoothChannels;
static std::unordered_map<IOBluetoothObjectID, std::string> completedBluetoothChannels;

void Async::Internal::init(unsigned int) { kq = call(FN(kqueue)); }

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

void Async::submitIOBluetooth(IOBluetoothObjectID id, CompletionResult& result) {
    pendingBluetoothChannels[id] = &result;
}

void Async::bluetoothComplete(IOBluetoothObjectID id, IOReturn status) {
    auto& result = *pendingBluetoothChannels[id];
    pendingBluetoothChannels.erase(id);

    result.error = status;
    result.coroHandle();
}

void Async::bluetoothReadComplete(IOBluetoothObjectID id, const char* data, size_t dataLen) {
    const auto& result = *pendingBluetoothChannels[id];
    pendingBluetoothChannels.erase(id);

    completedBluetoothChannels[id] = { data, dataLen };
    result.coroHandle();
}

std::string Async::getBluetoothReadResult(IOBluetoothObjectID id) {
    auto data = completedBluetoothChannels[id];
    completedBluetoothChannels.erase(id);
    return data;
}
#endif
