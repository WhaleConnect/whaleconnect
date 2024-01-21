// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_MACOS
#include <coroutine> // IWYU pragma: keep
#include <functional>

#include <BluetoothMacOS-Swift.h>
#include <IOKit/IOReturn.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

module sockets.delegates.server;
import net.netutils;
import os.async;
import os.async.platform;
import os.errcheck;
import os.error;
import sockets.incomingsocket;
import utils.task;

template <>
ServerAddress Delegates::Server<SocketTag::IP>::startServer(const Device& serverInfo) {
    ServerAddress result = NetUtils::startServer(serverInfo, handle);

    Async::prepSocket(*handle);
    return result;
}

template <>
Task<AcceptResult> Delegates::Server<SocketTag::IP>::accept() {
    co_await Async::run(std::bind_front(Async::submitKqueue, *handle, Async::IOType::Receive));

    sockaddr_storage client;
    auto clientAddr = reinterpret_cast<sockaddr*>(&client);
    socklen_t clientLen = sizeof(client);

    SocketHandle<SocketTag::IP> fd{ check(::accept(*handle, clientAddr, &clientLen)) };
    Device device = NetUtils::fromAddr(clientAddr, clientLen, ConnectionType::TCP);

    Async::prepSocket(*fd);
    co_return { device, std::make_unique<IncomingSocket<SocketTag::IP>>(std::move(fd)) };
}

template <>
Task<DgramRecvResult> Delegates::Server<SocketTag::IP>::recvFrom(size_t size) {
    sockaddr_storage from;
    auto fromAddr = reinterpret_cast<sockaddr*>(&from);
    socklen_t addrSize = sizeof(from);

    co_await Async::run(std::bind_front(Async::submitKqueue, *handle, Async::IOType::Receive));

    std::string data(size, 0);
    ssize_t recvLen = check(recvfrom(*handle, data.data(), data.size(), 0, fromAddr, &addrSize));
    data.resize(recvLen);

    co_return { NetUtils::fromAddr(fromAddr, addrSize, ConnectionType::UDP), data };
}

template <>
Task<> Delegates::Server<SocketTag::IP>::sendTo(Device device, std::string data) {
    auto addr = NetUtils::resolveAddr(device, false);

    co_await NetUtils::loopWithAddr(addr.get(), [this, &data](const AddrInfoType* resolveRes) -> Task<> {
        co_await Async::run(std::bind_front(Async::submitKqueue, *handle, Async::IOType::Send));
        check(sendto(*handle, data.data(), data.size(), 0, resolveRes->ai_addr, resolveRes->ai_addrlen));
    });
}

template <>
ServerAddress Delegates::Server<SocketTag::BT>::startServer(const Device& serverInfo) {
    handle.reset(BluetoothMacOS::makeBTServerHandle());

    bool isL2CAP = serverInfo.type == ConnectionType::L2CAP;
    check((*handle)->startServer(isL2CAP, serverInfo.port), checkTrue, [](bool) { return kIOReturnError; },
        System::ErrorType::IOReturn);
    return { serverInfo.port };
}

template <>
Task<AcceptResult> Delegates::Server<SocketTag::BT>::accept() {
    co_await Async::run(std::bind_front(Async::submitIOBluetooth, (*handle)->getHash(), Async::IOType::Receive),
        System::ErrorType::IOReturn);

    auto [device, newHandle] = Async::getBluetoothAcceptResult((*handle)->getHash());
    SocketHandle<SocketTag::BT> fd{ std::move(newHandle) };

    co_return { device, std::make_unique<IncomingSocket<SocketTag::BT>>(std::move(fd)) };
}
#endif
