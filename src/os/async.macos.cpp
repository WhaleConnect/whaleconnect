// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "async.hpp"

#include <cerrno>
#include <coroutine>
#include <cstdint>
#include <ctime>
#include <variant>

#include <sys/event.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "errcheck.hpp"
#include "utils/overload.hpp"

std::uint64_t getMapID(int s, std::int16_t filt) {
    //   | A kevent is identified by the (ident, filter) pair; there may only be one unique kevent per kqueue.
    // https://man.freebsd.org/cgi/man.cgi?kqueue
    //
    // There are two possible filters on sockets: EVFILT_READ and EVFILT_WRITE
    // This will be reduced into a single bit (read == 0; write == 1) and combined with the socket file descriptor to
    // produce a unique uint64_t for the pair.
    constexpr auto filtWrite = 1UL << 33;

    std::uint64_t filterBit = filt == EVFILT_WRITE ? filtWrite : 0;
    return static_cast<std::uint64_t>(s) | filterBit;
}

Async::EventLoop::EventLoop(unsigned int, unsigned int) : kq(check(kqueue())) {}

Async::EventLoop::~EventLoop() {
    close(kq);
}

void handleOperation(Async::PendingEventsMap& pendingEvents, std::vector<struct kevent>& events,
    const Async::Operation& next, std::size_t& numOperations) {
    auto submit = [&](int id, std::int16_t filt, Async::CompletionResult* result) {
        // Set EV_RECEIPT flag to get error status on kevent
        const std::uint16_t flags = EV_ADD | EV_ONESHOT | EV_RECEIPT;
        events.push_back({ static_cast<unsigned long>(id), filt, flags, 0, 0, result });
        pendingEvents[getMapID(id, filt)] = result;
        numOperations++;
    };

    Overload visitor{
        [&](const Async::Connect& op) { submit(op.handle, EVFILT_WRITE, op.result); },
        [&](const Async::Accept& op) { submit(op.handle, EVFILT_READ, op.result); },
        [&](const Async::Send& op) { submit(op.handle, EVFILT_WRITE, op.result); },
        [&](const Async::SendTo& op) { submit(op.handle, EVFILT_WRITE, op.result); },
        [&](const Async::Receive& op) { submit(op.handle, EVFILT_READ, op.result); },
        [&](const Async::ReceiveFrom& op) { submit(op.handle, EVFILT_READ, op.result); },
        [&](const Async::Shutdown& op) { shutdown(op.handle, SHUT_RDWR); },
        [&](const Async::Close& op) { close(op.handle); },
        [&](const Async::Cancel& op) {
            for (std::int16_t filt : { EVFILT_READ, EVFILT_WRITE }) {
                std::uint64_t mapID = getMapID(op.handle, filt);
                if (!pendingEvents.contains(mapID)) continue;

                // Cancelling means adding a kevent and removing a pending operation
                events.push_back({ static_cast<std::uintptr_t>(op.handle), filt, EV_DELETE, 0, 0, nullptr });
                numOperations--;

                auto& result = *pendingEvents[mapID];
                pendingEvents.erase(mapID);
                result.error = ECANCELED;
                result.coroHandle();
            }
        },
    };

    std::visit(visitor, next);
}

void Async::EventLoop::runOnce(bool wait) {
    if (operations.empty()) {
        if (numOperations == 0) return;
    } else {
        std::vector<struct kevent> events;

        for (const auto& i : operations) handleOperation(pendingEvents, events, i, numOperations);
        operations.clear();

        // Submit pending events from queue
        timespec timeout{ 0, 0 };
        if (kevent(kq, events.data(), events.size(), events.data(), events.size(), &timeout) == 0) return;

        for (const auto& i : events) {
            // Get events that set error status
            if (!(i.flags & EV_ERROR) || i.data == 0 || !i.udata) continue;

            // Return error status to calling coroutine
            pendingEvents.erase(getMapID(i.ident, i.filter));

            auto& result = *static_cast<Async::CompletionResult*>(i.udata);
            result.error = i.data;

            // Needs done check since results may have previously errored out from cancel operations
            if (!result.coroHandle.done()) result.coroHandle();
        }
    }

    struct kevent event {};

    timespec timeout{ 0, wait ? 200000000 : 0 };

    // Wait for one event from kqueue
    if (kevent(kq, nullptr, 0, &event, 1, &timeout) <= 0) return;
    numOperations--;

    // Pop an event from the map and get its completion result
    auto& result = *static_cast<CompletionResult*>(event.udata);

    // Set error and result
    if (event.flags & EV_EOF) result.error = static_cast<System::ErrorCode>(event.fflags);
    else result.res = static_cast<int>(event.data);

    pendingEvents.erase(getMapID(event.ident, event.filter));
    result.coroHandle();
}

void Async::prepSocket(int s) {
    int flags = check(fcntl(s, F_GETFL, 0));
    check(fcntl(s, F_SETFL, flags | O_NONBLOCK));
}
