// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_APPLE
#include "async_internal.hpp"

#include <sys/event.h>

#include "async.hpp"
#include "errcheck.hpp"

static int kq = -1;

bool Async::Internal::invalid() { return kq == -1; }

void Async::Internal::init() { kq = EXPECT_NONERROR(kqueue); }

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
    if ((event.filter == EVFILT_USER) && (event.ident == ASYNC_INTERRUPT)) return resultInterrupted();

    if (!event.udata) return resultError();
    auto& result = toResult(event.udata);
    if (event.flags & EV_EOF) result.error = static_cast<System::ErrorCode>(event.fflags);
    else result.res = static_cast<int>(event.data);

    return resultSuccess(result);
}

void Async::submitKqueue(int ident, int filter, CompletionResult& result) {
    // The EV_ONESHOT flag will delete the event once it is retrieved in one of the threads.
    // This ensures only one thread will wake up to handle the event.
    struct kevent event {};

    EV_SET(&event, ident, filter, EV_ADD | EV_ONESHOT, 0, 0, &result);
    EXPECT_NONERROR(kevent, kq, &event, 1, nullptr, 0, nullptr);
}
#endif
