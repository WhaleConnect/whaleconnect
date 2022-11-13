// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_LINUX
#include <liburing.h>

#include "async.hpp"
#include "errcheck.hpp"
#include "socket.hpp"

Socket::operator bool() const { return _handle != INVALID_SOCKET; }

void Socket::close() {
    io_uring_prep_cancel_fd(Async::getUringSQE(), _handle, IORING_ASYNC_CANCEL_ALL);
    io_uring_prep_shutdown(Async::getUringSQE(), _handle, SHUT_RDWR);
    io_uring_prep_close(Async::getUringSQE(), _handle);
    Async::submitRing();

    _handle = INVALID_SOCKET;
}

Task<> Socket::sendData(std::string data) const {
    co_await Async::run([this, &data](Async::CompletionResult& result) {
        io_uring_sqe* sqe = Async::getUringSQE();
        io_uring_prep_send(sqe, _handle, data.data(), data.size(), MSG_NOSIGNAL);
        io_uring_sqe_set_data(sqe, &result);
        Async::submitRing();
    });
}

Task<std::string> Socket::recvData() const {
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
