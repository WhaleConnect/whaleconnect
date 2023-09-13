// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_LINUX
#include <functional>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <liburing.h>
#include <sys/socket.h>

#include "client.hpp"

#include "net/netutils.hpp"
#include "os/async_linux.hpp"
#include "os/errcheck.hpp"

static void startConnect(int s, const sockaddr* addr, socklen_t len, Async::CompletionResult& result) {
    io_uring_sqe* sqe = Async::getUringSQE();
    io_uring_prep_connect(sqe, s, addr, len);
    io_uring_sqe_set_data(sqe, &result);
    Async::submitRing();
}

template <>
Task<> Delegates::Client<SocketTag::IP>::connect(const Device& device) {
    auto addr = NetUtils::resolveAddr(device);

    co_await NetUtils::loopWithAddr(addr.get(), [this](const AddrInfoType* result) -> Task<> {
        _handle.reset(call(FN(socket, result->ai_family, result->ai_socktype, result->ai_protocol)));
        co_await Async::run(std::bind_front(startConnect, *_handle, result->ai_addr, result->ai_addrlen));
    });
}

template <>
Task<> Delegates::Client<SocketTag::BT>::connect(const Device& device) {
    // Address of the device to connect to
    bdaddr_t bdaddr;
    str2ba(device.address.c_str(), &bdaddr);

    // The structure used depends on the protocol
    union {
        sockaddr_l2 addrL2;
        sockaddr_rc addrRC;
    } sAddrBT;

    socklen_t addrSize;

    // Set the appropriate sockaddr struct based on the protocol
    if (device.type == ConnectionType::RFCOMM) {
        _handle.reset(call(FN(socket, AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)));

        sAddrBT.addrRC = { AF_BLUETOOTH, bdaddr, static_cast<uint8_t>(device.port) };
        addrSize = sizeof(sAddrBT.addrRC);
    } else {
        _handle.reset(call(FN(socket, AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP)));

        sAddrBT.addrL2 = { AF_BLUETOOTH, htobs(device.port), bdaddr, 0, 0 };
        addrSize = sizeof(sAddrBT.addrL2);
    }

    co_await Async::run(std::bind_front(startConnect, *_handle, std::bit_cast<sockaddr*>(&sAddrBT), addrSize));
}
#endif
