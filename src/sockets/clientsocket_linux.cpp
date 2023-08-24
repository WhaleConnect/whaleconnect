// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_LINUX
#include <stdexcept>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <sys/socket.h>

#include "clientsocket.hpp"

#include "os/async_linux.hpp"
#include "os/errcheck.hpp"

template <>
void ClientSocketBT::_init() {
    using enum ConnectionType;
    const auto& device = _traits.device;

    // Determine the socket type
    int sockProto;
    int sockType;
    switch (device.type) {
        case L2CAP:
            sockProto = BTPROTO_L2CAP;
            sockType = SOCK_SEQPACKET;
            break;
        case RFCOMM:
            sockProto = BTPROTO_RFCOMM;
            sockType = SOCK_STREAM;
            break;
        default:
            throw std::invalid_argument{ "Invalid socket type" };
    }

    _handle = call(FN(socket, AF_BLUETOOTH, sockType, sockProto));
}
#endif
