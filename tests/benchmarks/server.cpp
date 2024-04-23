// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <atomic>
#include <chrono>
#include <cstdint>
#include <forward_list>
#include <iostream>
#include <string>

#include "net/enums.hpp"
#include "os/async.hpp"
#include "os/error.hpp"
#include "sockets/delegates/delegates.hpp"
#include "sockets/serversocket.hpp"
#include "utils/task.hpp"

Task<> recv(const SocketPtr& sock) {
    const char* response = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Length: 4\r\nContent-Type: text/html\r\n\r\ntest\r\n\r\n";

    while (true) {
        std::string data;

        try {
            auto result = co_await sock->recv(1024);
            if (result.closed) break;

            data = std::move(result.data);
        } catch (const System::SystemError& e) {
            if (e.isCanceled()) break;
        }

        if (data.ends_with("\r\n\r\n")) {
            co_await sock->send(response);
        }
    }
}

Task<> g(std::forward_list<SocketPtr>& clients, const ServerSocket<SocketTag::IP>& s, std::atomic_bool& pendingAccept) try {
    pendingAccept = true;
    auto [_, sock] = co_await s.accept();

    auto& client = clients.emplace_front(std::move(sock));
    pendingAccept = false;

    recv(client);
} catch (const System::SystemError& e) {
    if (e.isCanceled()) co_return;
}

void run() {
    const ServerSocket<SocketTag::IP> s;
    const std::uint16_t port = s.startServer({ ConnectionType::TCP, "", "0.0.0.0", 0 }).port;
    std::cout << "port = " << port << "\n";

    std::forward_list<SocketPtr> clients;
    std::atomic_bool pendingAccept = false;

    using namespace std::literals;
    const auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < 10s) {
        if (!pendingAccept) g(clients, s, pendingAccept);
    }

    s.cancelIO();
    for (const auto& i : clients) i->cancelIO();
}

int main() {
    Async::init(8, 2048);

    run();
}
