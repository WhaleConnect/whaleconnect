// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_LINUX
#include <liburing.h>

#include "os/async.hpp"
#include "os/errcheck.hpp"
#include "sockets/socket.hpp"

template <>
void SocketBase<SOCKET>::close() {
    io_uring_prep_cancel_fd(Async::getUringSQE(), _handle, IORING_ASYNC_CANCEL_ALL);
    io_uring_prep_shutdown(Async::getUringSQE(), _handle, SHUT_RDWR);
    io_uring_prep_close(Async::getUringSQE(), _handle);
    Async::submitRing();
    _release();
}

template <>
Task<> SocketBase<SOCKET>::sendData(std::string data) const {
    co_await Async::run([this, &data](Async::CompletionResult& result) {
        io_uring_sqe* sqe = Async::getUringSQE();
        io_uring_prep_send(sqe, _handle, data.data(), data.size(), MSG_NOSIGNAL);
        io_uring_sqe_set_data(sqe, &result);
        Async::submitRing();
    });
}

template <>
Task<std::string> SocketBase<SOCKET>::recvData() const {
    std::string data(_recvLen, 0);

    auto result = co_await Async::run([this, &data](Async::CompletionResult& result) {
        io_uring_sqe* sqe = Async::getUringSQE();
        io_uring_prep_recv(sqe, _handle, data.data(), _recvLen, MSG_NOSIGNAL);
        io_uring_sqe_set_data(sqe, &result);
        Async::submitRing();
    });

    data.resize(result.res);
    co_return data;
}
#endif
