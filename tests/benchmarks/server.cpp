// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include <cstdint>
#include <forward_list>
#include <iostream>
#include <string>

#include "net/enums.hpp"
#include "os/async.hpp"
#include "sockets/delegates/delegates.hpp"
#include "sockets/serversocket.hpp"
#include "utils/task.hpp"

Task<> b(const SocketPtr& sock) {
    const char* response = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Length: 4\r\nContent-Type: "
                           "text/html\r\n\r\ntest\r\n\r\n";

    co_await Async::queueToThread();

    while (true) {
        auto result = co_await sock->recv(1024);

        if (result.closed) co_return;

        if (result.data.ends_with("\r\n\r\n")) co_await sock->send(response);
    }
}

Task<> g(const ServerSocket<SocketTag::IP>& sock, bool& pendingAccept, std::forward_list<SocketPtr>& clients) {
    auto [_, client] = co_await sock.accept();
    pendingAccept = false;
    b(clients.emplace_front(std::move(client)));
}

void run() {
    const ServerSocket<SocketTag::IP> s;
    const std::uint16_t port = s.startServer({ ConnectionType::TCP, "", "0.0.0.0", 0 }).port;
    std::cout << "port = " << port << "\n";

    bool pendingAccept = false;
    std::forward_list<SocketPtr> clients;

    using namespace std::literals;
    const auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < 10s) {
        Async::handleEvents();
        if (!pendingAccept) {
            pendingAccept = true;
            g(s, pendingAccept, clients);
        }
    }

    s.cancelIO();
}

int main() {
    Async::init(4, 2048);

    run();
}
