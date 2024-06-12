// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <latch>
#include <list>

#include "net/enums.hpp"
#include "os/async.hpp"
#include "os/error.hpp"
#include "sockets/delegates/delegates.hpp"
#include "sockets/serversocket.hpp"
#include "utils/task.hpp"

thread_local std::list<SocketPtr> clients;

Task<> loop(SocketPtr& client) {
    static const char* response = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Length: 4\r\nContent-Type: "
                                  "text/html\r\n\r\ntest\r\n\r\n";

    co_await Async::queueToThread();
    SocketPtr& sock = clients.emplace_front(std::move(client));

    while (true) {
        try {
            auto result = co_await sock->recv(1024);

            if (result.closed) break;

            if (result.data.ends_with("\r\n\r\n")) co_await sock->send(response);
        } catch (const System::SystemError& e) {
            break;
        }
    }
}

Task<> accept(const ServerSocket<SocketTag::IP>& sock, bool& pendingAccept) {
    auto [_, client] = co_await sock.accept();
    pendingAccept = false;
    co_await loop(client);
}

void run() {
    const ServerSocket<SocketTag::IP> s;
    const std::uint16_t port = s.startServer({ ConnectionType::TCP, "", "0.0.0.0", 0 }).port;
    std::cout << "port = " << port << "\n";

    bool pendingAccept = false;

    // Run for 10 seconds
    using namespace std::literals;
    const auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < 10s) {
        Async::handleEvents();
        if (!pendingAccept) {
            pendingAccept = true;
            accept(s, pendingAccept);
        }
    }
}

int main(int argc, char** argv) {
    // Get number of threads from first command line argument
    unsigned int numThreads = 0;
    if (argc > 1) {
        char* arg = argv[1];
        std::from_chars_result res = std::from_chars(arg, arg + std::strlen(arg), numThreads);
        if (res.ec != std::errc{}) std::cout << "Invalid number of threads specified.\n";
    }

    unsigned int realNumThreads = Async::init(numThreads, 2048);
    std::cout << "Running with " << realNumThreads << " threads.\n";

    run();

    // Cancel remaining work on all threads
    std::latch threadWaiter{ realNumThreads - 1 };
    Async::queueToAllThreads([&threadWaiter]() {
        for (auto i = clients.begin(); i != clients.end(); i++) (*i)->cancelIO();

        clients.clear();
        threadWaiter.count_down();
    });

    threadWaiter.wait();
}
