// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_LINUX
#include <liburing.h>

#include "os/async.hpp"
#include "os/errcheck.hpp"
#include "sockets/socket.hpp"

template <auto Tag>
void Socket<Tag>::close() {
    io_uring_prep_shutdown(Async::getUringSQE(), _handle, SHUT_RDWR);
    io_uring_prep_close(Async::getUringSQE(), _handle);
    Async::submitRing();
    _release();
}

template <auto Tag>
Task<> WritableSocket<Tag>::send(std::string data) const {
    co_await Async::run([this, &data](Async::CompletionResult& result) {
        io_uring_sqe* sqe = Async::getUringSQE();
        io_uring_prep_send(sqe, this->_get(), data.data(), data.size(), MSG_NOSIGNAL);
        io_uring_sqe_set_data(sqe, &result);
        Async::submitRing();
    });
}

template <auto Tag>
Task<std::string> WritableSocket<Tag>::recv() const {
    std::string data(_recvLen, 0);

    auto result = co_await Async::run([this, &data](Async::CompletionResult& result) {
        io_uring_sqe* sqe = Async::getUringSQE();
        io_uring_prep_recv(sqe, this->_get(), data.data(), _recvLen, MSG_NOSIGNAL);
        io_uring_sqe_set_data(sqe, &result);
        Async::submitRing();
    });

    data.resize(result.res);
    co_return data;
}

template <auto Tag>
void WritableSocket<Tag>::cancelIO() const {
    io_uring_prep_cancel_fd(Async::getUringSQE(), this->_get(), IORING_ASYNC_CANCEL_ALL);
    Async::submitRing();
}

template void Socket<SocketTag::IP>::close();
template Task<> WritableSocket<SocketTag::IP>::send(std::string) const;
template Task<std::string> WritableSocket<SocketTag::IP>::recv() const;
template void WritableSocket<SocketTag::IP>::cancelIO() const;

template void Socket<SocketTag::BT>::close();
template Task<> WritableSocket<SocketTag::BT>::send(std::string) const;
template Task<std::string> WritableSocket<SocketTag::BT>::recv() const;
template void WritableSocket<SocketTag::BT>::cancelIO() const;
#endif
