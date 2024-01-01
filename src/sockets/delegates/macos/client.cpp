// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_MACOS
#include <coroutine> // IWYU pragma: keep
#include <functional>

#include <BluetoothMacOS-Swift.h>
#include <IOKit/IOReturn.h>
#include <sys/event.h>
#include <sys/socket.h>

#include "os/check.hpp"

module sockets.delegates.client;
import net.device;
import net.enums;
import net.netutils;
import os.async;
import os.async.platform;
import os.errcheck;
import os.error;

template <>
Task<> Delegates::Client<SocketTag::IP>::connect(Device device) {
    auto addr = NetUtils::resolveAddr(device);

    co_await NetUtils::loopWithAddr(addr.get(), [this](const AddrInfoType* result) -> Task<> {
        handle.reset(CHECK(::socket(result->ai_family, result->ai_socktype, result->ai_protocol)));

        Async::prepSocket(*handle);

        // Start connect
        CHECK(::connect(*handle, result->ai_addr, result->ai_addrlen));
        co_await Async::run(std::bind_front(Async::submitKqueue, *handle, Async::IOType::Send));
    });
}

template <>
Task<> Delegates::Client<SocketTag::BT>::connect(Device device) {
    bool isL2CAP;

    using enum ConnectionType;
    switch (device.type) {
        case L2CAP:
            isL2CAP = true;
            break;
        case RFCOMM:
            isL2CAP = false;
            break;
        default:
            throw System::SystemError{ kIOReturnUnsupported, System::ErrorType::IOReturn, "connect" };
    }

    // Init handle
    auto newHandle = CHECK(
        BluetoothMacOS::makeBTHandle(device.address, device.port, isL2CAP),
        [](const BluetoothMacOS::BTHandleResult& result) { return result.getResult() == kIOReturnSuccess; },
        [](const BluetoothMacOS::BTHandleResult& result) { return result.getResult(); }, System::ErrorType::IOReturn)
                         .getHandle()[0];

    handle.reset(newHandle);
    co_await Async::run(std::bind_front(Async::submitIOBluetooth, (*handle)->getHash(), Async::IOType::Send),
        System::ErrorType::IOReturn);
}
#endif
