// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_LINUX
#include <stdexcept>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <liburing.h>

#include "os/async.hpp"
#include "os/errcheck.hpp"
#include "sockets/clientsocket.hpp"

static void startConnect(int s, sockaddr* addr, socklen_t len, Async::CompletionResult& result) {
    io_uring_sqe* sqe = Async::getUringSQE();
    io_uring_prep_connect(sqe, s, addr, len);
    io_uring_sqe_set_data(sqe, &result);
    Async::submitRing();
}

template <>
Task<> ClientSocket<SocketTag::IP>::connect() const {
    co_await Async::run(std::bind_front(startConnect, _get(), _addr->ai_addr, _addr->ai_addrlen));
}

template <>
std::unique_ptr<ClientSocket<SocketTag::BT>> createClientSocket(const Device& device) {
    using enum ConnectionType;

    // Determine the socket type
    int sockType;
    switch (device.type) {
    case L2CAPStream:
    case RFCOMM: sockType = SOCK_STREAM; break;
    case L2CAPDgram: sockType = SOCK_DGRAM; break;
    case L2CAPSeqPacket: sockType = SOCK_SEQPACKET; break;
    default: throw std::invalid_argument{ "Invalid socket type" }; // Should never get here since this function is used internally.
    }

    // Determine the socket protocol
    int sockProto = (device.type == RFCOMM) ? BTPROTO_RFCOMM : BTPROTO_L2CAP;

    auto fd = call(FN(socket, AF_BLUETOOTH, sockType, sockProto));
    return std::make_unique<ClientSocket<SocketTag::BT>>(fd, device);
}

template <>
Task<> ClientSocket<SocketTag::BT>::connect() const {
    // Address of the device to connect to
    bdaddr_t bdaddr;
    str2ba(_device.address.c_str(), &bdaddr);

    // The structure used depends on the protocol
    union {
        sockaddr_l2 addrL2;
        sockaddr_rc addrRC;
    } sAddrBT;

    socklen_t addrSize;

    // Set the appropriate sockaddr struct based on the protocol
    if (_device.type == ConnectionType::RFCOMM) {
        sAddrBT.addrRC = { AF_BLUETOOTH, bdaddr, static_cast<uint8_t>(_device.port) };
        addrSize = sizeof(sAddrBT.addrRC);
    } else {
        sAddrBT.addrL2 = { AF_BLUETOOTH, htobs(_device.port), bdaddr, 0, 0 };
        addrSize = sizeof(sAddrBT.addrL2);
    }

    co_await Async::run(std::bind_front(startConnect, _get(), std::bit_cast<sockaddr*>(&sAddrBT), addrSize));
}
#endif
