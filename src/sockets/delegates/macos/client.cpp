// Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sockets/delegates/client.hpp"

#include <functional>
#include <utility>

#include <BluetoothMacOS-Swift.h>
#include <sys/socket.h>

#include "net/device.hpp"
#include "net/enums.hpp"
#include "net/netutils.hpp"
#include "os/async.hpp"
#include "os/bluetooth.hpp"
#include "os/errcheck.hpp"
#include "os/error.hpp"

template <>
Task<> Delegates::Client<SocketTag::IP>::connect(Device device) {
    auto addr = NetUtils::resolveAddr(device);

    co_await NetUtils::loopWithAddr(addr.get(), [this](const AddrInfoType* result) -> Task<> {
        handle.reset(check(::socket(result->ai_family, result->ai_socktype, result->ai_protocol)));

        Async::prepSocket(*handle);

        // Start connect
        check(::connect(*handle, result->ai_addr, result->ai_addrlen));
        co_await Async::run([this](Async::CompletionResult& result) {
            Async::submit(Async::Connect{ { *handle, &result } });
        });
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
            std::unreachable();
    }

    // Init handle
    auto newHandle = check(
        BluetoothMacOS::makeBTHandle(device.address, device.port, isL2CAP),
        [](const BluetoothMacOS::BTHandleResult& result) { return result.getResult() == kIOReturnSuccess; },
        [](const BluetoothMacOS::BTHandleResult& result) { return result.getResult(); }, System::ErrorType::IOReturn)
                         .getHandle()[0];

    handle.reset(newHandle);
    co_await Async::run(std::bind_front(AsyncBT::submit, (*handle)->getHash(), IOType::Send),
        System::ErrorType::IOReturn);
}
