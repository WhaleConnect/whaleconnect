// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

#include "helpers/helpers.hpp"
#include "net/enums.hpp"
#include "os/error.hpp"
#include "sockets/clientsocket.hpp"
#include "utils/settingsparser.hpp"
#include "utils/task.hpp"

// Matcher to check if a SystemError corresponds to a cancellation error.
struct CancellationMatcher : Catch::Matchers::MatcherGenericBase {
    bool match(const System::SystemError& e) const {
        return e.isCanceled();
    }

    std::string describe() const override {
        return "Is a cancellation error";
    }
};

TEST_CASE("Cancellation") {
    SettingsParser parser;
    parser.load(SETTINGS_FILE);

    const auto v4Addr = parser.get<std::string>("ip", "v4");
    const auto tcpPort = parser.get<std::uint16_t>("ip", "tcpPort");

    // Create IPv4 TCP socket
    ClientSocketIP sock;

    // Connect
    runSync([&]() -> Task<> { co_await sock.connect({ ConnectionType::TCP, "", v4Addr, tcpPort }); });

    // Create a separate thread to briefly wait, then cancel I/O while recv() is pending
    std::thread cancelThread{ [&sock] {
        using namespace std::literals;

        std::this_thread::sleep_for(20ms);
        sock.cancelIO();
    } };

    // Start a receive operation
    // It should be interrupted by the second thread and throw an exception
    auto recvOperation = [&sock]() -> Task<> { co_await sock.recv(4); };

    CHECK_THROWS_MATCHES(runSync(recvOperation), System::SystemError, CancellationMatcher{});

    cancelThread.join();
}
