// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_APPLE
#include <functional>

#include <sys/event.h>
#include <sys/socket.h>

#include "os/async.hpp"
#include "os/errcheck.hpp"
#include "plat_macos_objc/channel.hpp"
#include "sockets/socket.hpp"

template <>
void Socket<SocketTag::IP>::close() {
    shutdown(_handle, SHUT_RDWR);
    ::close(_handle);
    _release();
}

template <>
Task<> WritableSocket<SocketTag::IP>::send(std::string data) const {
    co_await Async::run(std::bind_front(Async::submitKqueue, _handle, EVFILT_WRITE));

    call(FN(::send, _handle, data.data(), data.size(), 0));
}

template <>
Task<std::string> WritableSocket<SocketTag::IP>::recv() const {
    co_await Async::run(std::bind_front(Async::submitKqueue, _handle, EVFILT_READ));

    std::string data(_recvLen, 0);
    ssize_t recvLen = call(FN(::recv, _handle, data.data(), data.size(), 0));

    data.resize(recvLen);
    co_return data;
}

template <>
void WritableSocket<SocketTag::IP>::cancelIO() const {
    Async::cancelPending(_handle);
}

template <>
void Socket<SocketTag::BT>::close() {
    if (_handle.type == ConnectionType::RFCOMM) ObjC::closeRFCOMMChannel(*_handle);
    else ObjC::closeL2CAPChannel(*_handle);

    _release();
}

template <>
Task<> WritableSocket<SocketTag::BT>::send(std::string) const {
    co_return;
}

template <>
Task<std::string> WritableSocket<SocketTag::BT>::recv() const {
    co_return "";
}

template <>
void WritableSocket<SocketTag::BT>::cancelIO() const {}
#endif
