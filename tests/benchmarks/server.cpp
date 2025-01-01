// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
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

struct Client {
    SocketPtr sock;
    bool done;

    Client(SocketPtr&& sock, bool done) : sock(std::move(sock)), done(done) {}
};

thread_local std::list<Client> clients;

Task<> loop(SocketPtr& ptr) {
    static const char* response = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Length: 4\r\nContent-Type: "
                                  "text/html\r\n\r\ntest\r\n\r\n";

    co_await Async::queueToThread();
    Client& client = clients.emplace_front(std::move(ptr), false);

    while (true) {
        try {
            auto result = co_await client.sock->recv(1024);
            if (result.closed) break;

            if (result.data.ends_with("\r\n\r\n")) co_await client.sock->send(response);
        } catch (const System::SystemError&) {
            break;
        }
    }
    client.done = true;
}

Task<> accept(const ServerSocket<SocketTag::IP>& sock, bool& pendingAccept) try {
    auto [_, client] = co_await sock.accept();
    pendingAccept = false;
    co_await loop(client);
} catch (const System::SystemError&) {
    pendingAccept = false;
}

void run() {
    const ServerSocket<SocketTag::IP> s;
    const std::uint16_t port = s.startServer({ ConnectionType::TCP, "", "0.0.0.0", 3000 }).port;
    std::cout << "port = " << port << "\n";

    bool pendingAccept = false;

    // Run for 10 seconds
    using namespace std::literals;
    const auto start = std::chrono::steady_clock::now();
    while (true) {
        bool timeout = std::chrono::steady_clock::now() - start > 10s;
        if (timeout) {
            s.cancelIO();
            s.close();
        }

        Async::handleEvents();
        if (timeout) break;

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
    Async::queueToThreadEx({}, []() -> Task<bool> {
        for (auto i = clients.begin(); i != clients.end(); i++)
            if (!i->done) i->sock->cancelIO();

        co_return false;
    });

    std::latch threadWaiter{ realNumThreads - 1 };
    Async::queueToThreadEx({}, [&threadWaiter]() -> Task<bool> {
        if (clients.empty()) {
            threadWaiter.count_down();
            co_return false;
        }

        std::erase_if(clients, [](const Client& client) { return client.done; });
        co_return true;
    });

    threadWaiter.wait();
    Async::cleanup();
}
