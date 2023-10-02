// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_LINUX
#include <coroutine>
#include <functional>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <liburing.h>
#include <sys/socket.h>

#include "os/check.hpp"

module sockets.delegates.client;
import net.device;
import net.enums;
import net.netutils;
import os.async;
import os.async.platform;
import os.errcheck;

void startConnect(int s, const sockaddr* addr, socklen_t len, Async::CompletionResult& result) {
    io_uring_sqe* sqe = Async::getUringSQE();
    io_uring_prep_connect(sqe, s, addr, len);
    io_uring_sqe_set_data(sqe, &result);
    Async::submitRing();
}

template <>
Task<> Delegates::Client<SocketTag::IP>::connect(Device device) {
    auto addr = NetUtils::resolveAddr(device);

    co_await NetUtils::loopWithAddr(addr.get(), [this](const AddrInfoType* result) -> Task<> {
        handle.reset(CHECK(socket(result->ai_family, result->ai_socktype, result->ai_protocol)));
        co_await Async::run(std::bind_front(startConnect, *handle, result->ai_addr, result->ai_addrlen));
    });
}

template <>
Task<> Delegates::Client<SocketTag::BT>::connect(Device device) {
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
        handle.reset(CHECK(socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)));

        sAddrBT.addrRC = { AF_BLUETOOTH, bdaddr, static_cast<uint8_t>(device.port) };
        addrSize = sizeof(sAddrBT.addrRC);
    } else {
        handle.reset(CHECK(socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP)));

        sAddrBT.addrL2 = { AF_BLUETOOTH, htobs(device.port), bdaddr, 0, 0 };
        addrSize = sizeof(sAddrBT.addrL2);
    }

    co_await Async::run(std::bind_front(startConnect, *handle, std::bit_cast<sockaddr*>(&sAddrBT), addrSize));
}
#endif
