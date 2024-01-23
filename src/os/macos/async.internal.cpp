// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// kqueue and IOBluetooth only handle notifications for I/O, so completion queues must be managed manually.

module;
#include <array>
#include <vector>
#include <optional>

#include <IOKit/IOReturn.h>
#include <sys/event.h>
#include <sys/fcntl.h>

module os.async.internal;
import os.async;
import os.async.platform;
import os.async.platform.internal;
import os.errcheck;
import os.error;

// Bitmask to extract file descriptors from the ASYNC_* idenfifiers
constexpr auto socketIDMask = 0xFFFFFFFFUL;

// Pops and cancels a pending operation.
Async::Internal::OptCompletionResult cancelOne(uint64_t id, Async::Internal::SocketQueueMap& map,
    Async::IOType ioType) {
    // Completion result pointing to suspended coroutine
    auto pending = popPending(id, map, ioType);
    if (!pending) return std::nullopt;

    // Set error and return coroutine handle
    auto& result = *pending;
    result->error = ECANCELED;
    return result;
}

// Removes kqueue events for a socket.
void deleteKqueueEvents(int kq, int id) {
    std::array<struct kevent, 2> events;

    EV_SET(&events[0], id, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    EV_SET(&events[1], id, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);

    kevent(kq, events.data(), events.size(), nullptr, 0, nullptr);
}

void Async::Internal::init(unsigned int numThreads, unsigned int) {
    // Populate the vector of kqueues
    kqs.reserve(numThreads);

    for (unsigned int i = 0; i < numThreads; i++) kqs.push_back(check(kqueue()));
}

void Async::Internal::stopThreads(unsigned int) {
    // Submit events to wake up all threads
    for (auto kq : kqs) {
        struct kevent event {};

        EV_SET(&event, ASYNC_INTERRUPT, EVFILT_USER, EV_ADD | EV_ONESHOT, NOTE_TRIGGER, 0, nullptr);
        kevent(kq, &event, 1, nullptr, 0, nullptr);
    }
}

void Async::Internal::cleanup() {
    // No cleanup needed
}

void Async::Internal::worker(unsigned int threadNum) {
    SocketQueueMap sockets;

    const int kq = kqs[threadNum];
    while (true) {
        struct kevent event {};

        // Wait for new event
        if (kevent(kq, nullptr, 0, &event, 1, nullptr) == -1) continue;

        // Handle user events
        if (event.filter == EVFILT_USER) {
            const auto ident = event.ident;

            if (ident == ASYNC_INTERRUPT) break;

            if (ident & ASYNC_ADD) {
                // Add completion result to queue
                auto id = ident & socketIDMask;
                auto ioType = static_cast<IOType>(event.fflags & NOTE_FFLAGSMASK);
                auto& result = *reinterpret_cast<CompletionResult*>(event.udata);

                addPending(id, sockets, ioType, result);
            } else if (ident & ASYNC_CANCEL) {
                // Cancel pending operations
                using enum Async::IOType;

                // Extract file descriptor from identifier and remove kqueue events
                int id = event.ident & socketIDMask;
                deleteKqueueEvents(kq, id);

                // Cancel receive and send operations in order
                while (const auto recvCancel = cancelOne(id, sockets, Receive)) (*recvCancel)->coroHandle();

                while (const auto sendCancel = cancelOne(id, sockets, Send)) (*sendCancel)->coroHandle();
            }

            continue;
        }

        // Get I/O type from user data pointer
        auto ioTypeInt = static_cast<int>(reinterpret_cast<uint64_t>(event.udata));
        auto ioType = static_cast<IOType>(ioTypeInt);

        // Pop an event from the queue and get its completion result
        auto resultOpt = popPending(event.ident, sockets, ioType);
        if (!resultOpt) continue;

        auto& result = **resultOpt;

        // Set error and result
        if (event.flags & EV_EOF) result.error = static_cast<System::ErrorCode>(event.fflags);
        else result.res = static_cast<int>(event.data);

        queueForCompletion(result);
    }
}
