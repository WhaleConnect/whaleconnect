// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "async.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <variant>

#include <liburing.h>
#include <sys/socket.h>

#include "os/errcheck.hpp"
#include "utils/overload.hpp"

void handleOperation(io_uring& ring, const Async::Operation& next) {
    io_uring_sqe* sqe = io_uring_get_sqe(&ring);

    Overload visitor{
        [=](const Async::Connect& op) {
            io_uring_prep_connect(sqe, op.handle, op.addr, op.addrLen);
            io_uring_sqe_set_data(sqe, op.result);
        },
        [=](const Async::Accept& op) {
            io_uring_prep_accept(sqe, op.handle, op.addr, op.addrLen, 0);
            io_uring_sqe_set_data(sqe, op.result);
        },
        [=](const Async::Send& op) {
            io_uring_prep_send(sqe, op.handle, op.data.data(), op.data.size(), MSG_NOSIGNAL);
            io_uring_sqe_set_data(sqe, op.result);
        },
        [=](const Async::SendTo& op) {
            io_uring_prep_sendto(sqe, op.handle, op.data.data(), op.data.size(), MSG_NOSIGNAL, op.addr, op.addrLen);
            io_uring_sqe_set_data(sqe, op.result);
        },
        [=](const Async::Receive& op) {
            io_uring_prep_recv(sqe, op.handle, op.data.data(), op.data.size(), MSG_NOSIGNAL);
            io_uring_sqe_set_data(sqe, op.result);
        },
        [=](const Async::ReceiveFrom& op) {
            io_uring_prep_recvmsg(sqe, op.handle, op.msg, MSG_NOSIGNAL);
            io_uring_sqe_set_data(sqe, op.result);
        },
        [=](const Async::Shutdown& op) {
            io_uring_prep_shutdown(sqe, op.handle, SHUT_RDWR);
            io_uring_sqe_set_data(sqe, nullptr);
        },
        [=](const Async::Close& op) {
            io_uring_prep_close(sqe, op.handle);
            io_uring_sqe_set_data(sqe, nullptr);
        },
        [=](const Async::Cancel& op) {
            io_uring_prep_cancel_fd(sqe, op.handle, IORING_ASYNC_CANCEL_ALL);
            io_uring_sqe_set_data(sqe, nullptr);
        },
    };

    std::visit(visitor, next);
}

Async::WorkerThread::WorkerThread(unsigned int, unsigned int queueEntries) {
    check(io_uring_queue_init(queueEntries, &ring, 0), checkZero, useReturnCodeNeg);

    // Initialized after io_uring
    thread = std::thread{ &Async::WorkerThread::eventLoop, this };
}

Async::WorkerThread::~WorkerThread() {
    io_uring_sqe* sqe = io_uring_get_sqe(&ring);
    io_uring_prep_nop(sqe);
    io_uring_sqe_set_data64(sqe, asyncInterrupt);
    io_uring_submit(&ring);
    numOperations.fetch_add(1, std::memory_order_relaxed);
    operationsPending.store(true, std::memory_order_relaxed);
    operationsPending.notify_one();

    thread.join();
    io_uring_queue_exit(&ring);
}

void Async::WorkerThread::processOperations() {
    std::scoped_lock lock{ operationsMtx };
    numOperations.fetch_add(operations.size(), std::memory_order_relaxed);
    while (!operations.empty()) {
        auto next = operations.front();
        operations.pop();
        handleOperation(ring, next);
    }
}

void Async::WorkerThread::eventLoop() {
    while (true) {
        io_uring_cqe* cqe;

        bool expected = true;
        if (operationsPending.compare_exchange_weak(expected, false, std::memory_order_relaxed)) {
            // There are queued operations, process them
            processOperations();

            // Submit to io_uring and wait for next CQE
            if (io_uring_submit_and_wait(&ring, 1) != 0) continue;
            if (io_uring_peek_cqe(&ring, &cqe) != 0) continue;
        } else if (numOperations.load(std::memory_order_relaxed) == 0) {
            // There are no queued operations and nothing pending in io_uring, thread is idle
            operationsPending.wait(false, std::memory_order_relaxed);
            continue;
        } else if (io_uring_wait_cqe(&ring, &cqe) != 0) {
            // There are no queued operations but there are operations pending in io_uring, wait for them
            continue;
        }

        numOperations.fetch_sub(1, std::memory_order_relaxed);
        void* userData = io_uring_cqe_get_data(cqe);
        io_uring_cqe_seen(&ring, cqe);

        if (userData == nullptr) continue;

        // Check for interrupt
        if (reinterpret_cast<std::uint64_t>(userData) == asyncInterrupt) break;

        // Fill in completion result information
        auto& result = *reinterpret_cast<CompletionResult*>(userData);
        if (cqe->res < 0) result.error = -cqe->res;
        else result.res = cqe->res;

        result.coroHandle();
    }
}
