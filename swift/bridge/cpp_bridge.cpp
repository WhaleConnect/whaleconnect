// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include "cpp_bridge.hpp"

import os.async.platform;
import net.device;
import net.enums;
import os.error;

void clearDataQueue(unsigned long id) {
    Async::clearBluetoothDataQueue(id);
}

void newData(unsigned long id, const char* data, size_t dataLen) {
    Async::bluetoothReadComplete(id, data, dataLen);
}

void outgoingComplete(unsigned long id, IOReturn status) {
    Async::bluetoothComplete(id, Async::IOType::Send, status);
}

void acceptComplete(unsigned long id, bool isL2CAP, const std::string& name, const std::string& addr, uint16_t port) {
    using enum ConnectionType;
    Async::bluetoothAcceptComplete(id, { isL2CAP ? L2CAP : RFCOMM, name, addr, port });
}

void closed(unsigned long id) {
    Async::bluetoothClosed(id);
}
#endif
