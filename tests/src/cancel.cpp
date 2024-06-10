// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>

#include <catch2/catch_test_macros.hpp>

#include "helpers/helpers.hpp"
#include "net/enums.hpp"
#include "os/async.hpp"
#include "os/error.hpp"
#include "sockets/clientsocket.hpp"
#include "utils/settingsparser.hpp"
#include "utils/task.hpp"

TEST_CASE("Cancellation") {
    SettingsParser parser;
    parser.load(SETTINGS_FILE);

    const auto v4Addr = parser.get<std::string>("ip", "v4");
    const auto tcpPort = parser.get<std::uint16_t>("ip", "tcpPort");

    // Create IPv4 TCP socket
    ClientSocketIP sock;

    // Connect
    runSync([&]() -> Task<> { co_await sock.connect({ ConnectionType::TCP, "", v4Addr, tcpPort }); });

    bool running = true;
    [&]() -> Task<> {
        try {
            co_await sock.recv(4);
        } catch (const System::SystemError& e) {
            CHECK(e.isCanceled());
            running = false;
        }
    }();

    int iterations = 0;
    while (running) {
        using namespace std::literals;

        Async::handleEvents(false);
        if (iterations == 5) sock.cancelIO();

        iterations++;
    }
}
