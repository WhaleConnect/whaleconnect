// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_LINUX
#include "net_internal.hpp"

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <liburing.h>

#include "async.hpp"
#include "net.hpp"

void Net::init() {}

void Net::cleanup() {}

void Net::Internal::startConnect(SOCKET s, const sockaddr* addr, socklen_t len, bool, Async::CompletionResult& result) {
    io_uring_sqe* sqe = Async::getUringSQE();
    io_uring_prep_connect(sqe, s, addr, len);
    io_uring_sqe_set_data(sqe, &result);
    Async::submitRing();
}

void Net::Internal::finalizeConnect(SOCKET, bool) {}

Task<Socket> Net::Internal::createClientSocketBT(const DeviceData& data) {
    using enum Net::ConnectionType;

    // Determine the socket type
    int sockType;
    switch (type) {
    case L2CAPStream:
    case RFCOMM: sockType = SOCK_STREAM; break;
    case L2CAPDgram: sockType = SOCK_DGRAM; break;
    case L2CAPSeqPacket: sockType = SOCK_SEQPACKET; break;
    default: co_return INVALID_SOCKET; // Should never get here since this function is used internally.
    }

    // Determine the socket protocol
    int sockProto = (type == RFCOMM) ? BTPROTO_RFCOMM : BTPROTO_L2CAP;

    SOCKET fd = EXPECT_NONERROR(socket, AF_BLUETOOTH, sockType, sockProto);
    Socket ret{ fd };

    // Address of the device to connect to
    bdaddr_t bdaddr;
    str2ba(data.address.c_str(), &bdaddr);

    // The structure used depends on the protocol
    union {
        sockaddr_l2 addrL2;
        sockaddr_rc addrRC;
    } sAddrBT;

    socklen_t addrSize;

    // Set the appropriate sockaddr struct based on the protocol
    if (data.type == RFCOMM) {
        sAddrBT.addrRC = { AF_BLUETOOTH, bdaddr, static_cast<uint8_t>(data.port) };
        addrSize = sizeof(sAddrBT.addrRC);
    } else {
        sAddrBT.addrL2 = { AF_BLUETOOTH, htobs(data.port), bdaddr, 0, 0 };
        addrSize = sizeof(sAddrBT.addrL2);
    }

    bool isDgram = (data.type == L2CAPDgram);

    co_await Async::run(std::bind_front(startConnect, fd, &sAddrBT, addrSize, isDgram));
    finalizeConnect(fd, false);

    co_return std::move(ret);
}
#endif
