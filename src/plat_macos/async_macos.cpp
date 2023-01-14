// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_APPLE
#include "os/async_internal.hpp"

#include <mutex>
#include <unordered_map>

#include <sys/event.h>

#include "os/async.hpp"
#include "os/errcheck.hpp"

static constexpr auto ASYNC_CANCEL = 2;

static int kq = -1;
static std::unordered_map<int, Async::CompletionResult*> pendingSockets;
static std::mutex mapMutex;

void Async::Internal::init() { kq = call(FN(kqueue)); }

void Async::Internal::stopThreads() {
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

Async::Internal::WorkerResult Async::Internal::worker() {
    // Wait for new event
    struct kevent event {};

    int nev = kevent(kq, nullptr, 0, &event, 1, nullptr);
    if (nev < 0) return resultError();

    // Check for interrupt
    if (event.filter == EVFILT_USER) {
        if (event.ident == ASYNC_INTERRUPT) {
            return resultInterrupted();
        } else if (event.ident == ASYNC_CANCEL) {
            auto fd = static_cast<int>(event.data);

            std::scoped_lock lock{ mapMutex };
            if (!pendingSockets.contains(fd)) return { false, std::noop_coroutine() };

            auto& result = *pendingSockets[fd];
            pendingSockets.erase(fd);

            result.error = ECANCELED;
            return resultSuccess(result);
        }
    }

    if (!event.udata) return resultError();
    auto& result = toResult(event.udata);

    if (result.coroHandle.done()) return { false, std::noop_coroutine() };

    if (event.flags & EV_EOF) result.error = static_cast<System::ErrorCode>(event.fflags);
    else result.res = static_cast<int>(event.data);

    return resultSuccess(result);
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
#endif
