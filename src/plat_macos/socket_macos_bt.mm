// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <IOBluetooth/IOBluetooth.h>

#include "sockets/socket.hpp"

template <>
void Socket<SocketTag::BT>::close() {
    id channel = nullptr;

    if (_handle.type == ConnectionType::RFCOMM) channel = [IOBluetoothRFCOMMChannel withObjectID:*_handle];
    else [IOBluetoothL2CAPChannel withObjectID:*_handle];

    [channel closeChannel];

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
