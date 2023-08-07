// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_LINUX
#include <stdexcept>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>

#include "async_linux.hpp"

#include "os/errcheck.hpp"
#include "sockets/clientsocket.hpp"

template <>
void ClientSocket<SocketTag::BT>::_init() {
    using enum ConnectionType;
    const auto& device = _traits.device;

    // Determine the socket type
    int sockType;
    switch (device.type) {
        case L2CAPStream:
        case RFCOMM:
            sockType = SOCK_STREAM;
            break;
        case L2CAPDgram:
            sockType = SOCK_DGRAM;
            break;
        case L2CAPSeqPacket:
            sockType = SOCK_SEQPACKET;
            break;
        default:
            throw std::invalid_argument{ "Invalid socket type" };
    }

    // Determine the socket protocol
    int sockProto = (device.type == RFCOMM) ? BTPROTO_RFCOMM : BTPROTO_L2CAP;
    _handle = call(FN(socket, AF_BLUETOOTH, sockType, sockProto));
}
#endif
