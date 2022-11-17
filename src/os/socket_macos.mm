// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_APPLE
#include <bit>
#include <functional>

#include <IOBluetooth/IOBluetooth.h>
#include <sys/event.h>
#include <sys/socket.h>

#include "async.hpp"
#include "errcheck.hpp"
#include "net.hpp"
#include "socket.hpp"
#include "utils/overload.hpp"

Socket::operator bool() const {
    if (std::holds_alternative<std::monostate>(_handle)) return false;

    if (std::holds_alternative<SOCKET>(_handle) && (_getFd() == INVALID_SOCKET)) return false;

    return true;
}

void Socket::close() {
    Overload visitor{
        [](std::monostate) {},
        [](SOCKET s) {
            shutdown(s, SHUT_RDWR);
            ::close(s);
        },
        [](L2CAPChannelWrapper wrapper) {
            auto channel = [IOBluetoothL2CAPChannel withL2CAPChannelRef:wrapper.channel];
            [channel closeChannel];
        },
        [](RFCOMMChannelWrapper wrapper) {
            auto channel = [IOBluetoothRFCOMMChannel withRFCOMMChannelRef:wrapper.channel];
            [channel closeChannel];
        },
    };

    std::visit(visitor, _handle);
    _handle = std::monostate{};
}

Task<> Socket::send(std::string data) const {
    if (std::holds_alternative<SOCKET>(_handle)) {
        auto s = _getFd();

        co_await Async::run(std::bind_front(Async::submitKqueue, s, EVFILT_WRITE));

        EXPECT_NONERROR(::send, s, data.data(), data.size(), 0);
    }
}

Task<std::string> Socket::recv() const {
    if (std::holds_alternative<SOCKET>(_handle)) {
        auto s = _getFd();

        std::string data(_recvLen, 0);

        auto result = co_await Async::run(std::bind_front(Async::submitKqueue, s, EVFILT_READ));

        EXPECT_NONERROR(::recv, s, data.data(), data.size(), 0);

        data.resize(result.res);
        co_return data;
    }

    co_return {};
}
#endif
