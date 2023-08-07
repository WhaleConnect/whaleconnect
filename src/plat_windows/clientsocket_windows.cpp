// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include <WinSock2.h>
#include <ws2bth.h>

#include "os/errcheck.hpp"
#include "os/error.hpp"
#include "sockets/clientsocket.hpp"

template <>
void ClientSocket<SocketTag::BT>::_init() {
    // Only RFCOMM sockets are supported by the Microsoft Bluetooth stack on Windows
    if (_traits.device.type != ConnectionType::RFCOMM)
        throw System::SystemError{ WSAEPFNOSUPPORT, System::ErrorType::System, "socket" };

    _handle = call(FN(socket, AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM));
}
#endif
