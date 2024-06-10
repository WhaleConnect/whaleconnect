// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "async.hpp"

#include <variant>

#include <liburing.h>
#include <linux/time_types.h>
#include <sys/socket.h>

#include "errcheck.hpp"
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

Async::EventLoop::EventLoop(unsigned int, unsigned int queueEntries) {
    io_uring_params params{};
    params.flags = IORING_SETUP_SINGLE_ISSUER;

    check(io_uring_queue_init_params(queueEntries, &ring, &params), checkZero, useReturnCodeNeg);
}

Async::EventLoop::~EventLoop() {
    io_uring_queue_exit(&ring);
}

void Async::EventLoop::runOnce(bool wait) {
    __kernel_timespec timeout{ 0, wait ? 200000000 : 0 };
    io_uring_cqe* cqe = nullptr;

    if (operations.empty()) {
        if (numOperations == 0) return;

        if (io_uring_wait_cqe_timeout(&ring, &cqe, &timeout) < 0) return;
    } else {
        // There are queued operations, process them
        for (const auto& i : operations) handleOperation(ring, i);
        numOperations += operations.size();
        operations.clear();

        // Submit to io_uring and wait for next CQE
        if (io_uring_submit_and_wait_timeout(&ring, &cqe, 1, &timeout, nullptr) < 0) return;
    }

    if (!cqe) return;

    void* userData = io_uring_cqe_get_data(cqe);
    io_uring_cqe_seen(&ring, cqe);
    numOperations--;

    if (!userData) return;

    // Fill in completion result information
    auto& result = *reinterpret_cast<CompletionResult*>(userData);
    if (cqe->res < 0) result.error = -cqe->res;
    else result.res = cqe->res;

    result.coroHandle();
}

std::size_t Async::EventLoop::size() {
    return numOperations;
}
