// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <functional>
#include <memory>
#include <string>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <sys/socket.h>

#include "net/enums.hpp"
#include "net/netutils.hpp"
#include "os/async.hpp"
#include "os/errcheck.hpp"
#include "sockets/delegates/server.hpp"
#include "sockets/incomingsocket.hpp"
#include "utils/strings.hpp"
#include "utils/task.hpp"

void startAccept(int s, sockaddr* clientAddr, socklen_t& clientLen, Async::CompletionResult& result) {
    Async::submit(Async::Accept{ { s, &result }, clientAddr, &clientLen });
}

template <>
ServerAddress Delegates::Server<SocketTag::IP>::startServer(const Device& serverInfo) {
    return NetUtils::startServer(serverInfo, handle);
}

template <>
Task<AcceptResult> Delegates::Server<SocketTag::IP>::accept() {
    sockaddr_storage client;
    auto clientAddr = reinterpret_cast<sockaddr*>(&client);
    socklen_t clientLen = sizeof(client);

    auto acceptResult = co_await Async::run(std::bind_front(startAccept, *handle, clientAddr, clientLen));

    Device device = NetUtils::fromAddr(clientAddr, clientLen, ConnectionType::TCP);
    SocketHandle<SocketTag::IP> fd{ acceptResult.res };

    co_return { device, std::make_unique<IncomingSocket<SocketTag::IP>>(std::move(fd)) };
}

template <>
Task<DgramRecvResult> Delegates::Server<SocketTag::IP>::recvFrom(std::size_t size) {
    // io_uring currently does not support recvfrom so recvmsg must be used instead:
    // https://github.com/axboe/liburing/issues/397
    // https://github.com/axboe/liburing/discussions/581

    sockaddr_storage from;
    auto fromAddr = reinterpret_cast<sockaddr*>(&from);
    socklen_t len = sizeof(from);
    std::string data(size, 0);

    iovec iov{
        .iov_base = data.data(),
        .iov_len = data.size(),
    };

    msghdr msg{
        .msg_name = &from,
        .msg_namelen = len,
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = nullptr,
        .msg_controllen = 0,
        .msg_flags = 0,
    };

    auto recvResult = co_await Async::run([this, &msg](Async::CompletionResult& result) {
        Async::submit(Async::ReceiveFrom{ { *handle, &result }, &msg });
    });

    data.resize(recvResult.res);
    co_return { NetUtils::fromAddr(fromAddr, len, ConnectionType::UDP), data };
}

template <>
Task<> Delegates::Server<SocketTag::IP>::sendTo(Device device, std::string data) {
    auto addr = NetUtils::resolveAddr(device, false);

    co_await NetUtils::loopWithAddr(addr.get(), [this, &data](const AddrInfoType* resolveRes) -> Task<> {
        co_await Async::run([this, &data, &resolveRes](Async::CompletionResult& result) {
            Async::submit(Async::SendTo{ { *handle, &result }, data, resolveRes->ai_addr, resolveRes->ai_addrlen });
        });
    });
}

template <>
ServerAddress Delegates::Server<SocketTag::BT>::startServer(const Device& serverInfo) {
    bdaddr_t addrAny{};
    bool isRFCOMM = serverInfo.type == ConnectionType::RFCOMM;

    if (isRFCOMM) {
        handle.reset(check(socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)));

        sockaddr_rc addr{ AF_BLUETOOTH, addrAny, static_cast<std::uint8_t>(serverInfo.port) };
        check(bind(*handle, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));
    } else {
        handle.reset(check(socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP)));

        sockaddr_l2 addr{ AF_BLUETOOTH, htobs(serverInfo.port), addrAny, 0, 0 };
        check(bind(*handle, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));
    }

    sockaddr_storage serverAddr;
    socklen_t serverAddrLen = sizeof(serverAddr);

    check(listen(*handle, SOMAXCONN));
    check(getsockname(*handle, reinterpret_cast<sockaddr*>(&serverAddr), &serverAddrLen));

    std::uint16_t port = isRFCOMM ? reinterpret_cast<sockaddr_rc*>(&serverAddr)->rc_channel
                        : reinterpret_cast<sockaddr_l2*>(&serverAddr)->l2_psm;

    return { btohs(port), IPType::None };
}

template <>
Task<AcceptResult> Delegates::Server<SocketTag::BT>::accept() {
    int sockType;
    socklen_t sockTypeLen = sizeof(sockType);

    // Check if socket is RFCOMM or L2CAP
    check(getsockopt(*handle, SOL_SOCKET, SO_TYPE, &sockType, &sockTypeLen));

    Device device;
    bdaddr_t clientbdAddr;
    SocketHandle<SocketTag::BT> fd;
    if (sockType == SOCK_STREAM) {
        // RFCOMM socket (stream type)
        sockaddr_rc client;
        auto clientAddr = reinterpret_cast<sockaddr*>(&client);
        socklen_t clientLen = sizeof(client);

        auto acceptResult = co_await Async::run(std::bind_front(startAccept, *handle, clientAddr, clientLen));

        device = { ConnectionType::RFCOMM, "", "", client.rc_channel };
        clientbdAddr = client.rc_bdaddr;
        fd.reset(acceptResult.res);
    } else {
        // L2CAP socket (sequential packet type)
        sockaddr_l2 client;
        auto clientAddr = reinterpret_cast<sockaddr*>(&client);
        socklen_t clientLen = sizeof(client);

        auto acceptResult = co_await Async::run(std::bind_front(startAccept, *handle, clientAddr, clientLen));

        device = { ConnectionType::L2CAP, "", "", btohs(client.l2_psm) };
        clientbdAddr = client.l2_bdaddr;
        fd.reset(acceptResult.res);
    }

    // Get MAC address (textual length is 17 chars)
    device.address.resize(17);
    ba2str(&clientbdAddr, device.address.data());

    // Get device name
    device.name.resize(1024);
    int devID = check(hci_get_route(nullptr));

    // HCI socket uses same close call as a standard socket
    SocketHandle<SocketTag::BT> hciSock{ check(hci_open_dev(devID)) };

    check(hci_read_remote_name(*hciSock, &clientbdAddr, device.name.size(), device.name.data(), 0));
    Strings::stripNull(device.name);

    co_return { device, std::make_unique<IncomingSocket<SocketTag::BT>>(std::move(fd)) };
}
