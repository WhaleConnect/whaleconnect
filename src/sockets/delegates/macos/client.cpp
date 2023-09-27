// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_MACOS
#include <coroutine>
#include <functional>

#include <sys/event.h>
#include <sys/socket.h>

#include "objc/cpp_objc_bridge.hpp"
#include "os/fn.hpp"

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
        handle.reset(call(FN(socket, result->ai_family, result->ai_socktype, result->ai_protocol)));

        Async::prepSocket(*handle);

        // Start connect
        call(FN(::connect, *handle, result->ai_addr, result->ai_addrlen));
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

    // Init channel
    handle.reset(CppObjCBridge::Bluetooth::connect(device.address, device.port, isL2CAP));

    co_await Async::run(
        std::bind_front(Async::submitIOBluetooth, CppObjCBridge::getBTHandleHash(*handle), Async::IOType::Send),
        System::ErrorType::IOReturn);
}
#endif
