// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

#include "helpers/helpers.hpp"
#include "os/async.hpp"
#include "os/error.hpp"
#include "sockets/clientsocket.hpp"

const auto settings = loadSettings();
const auto ipSettings = settings["ip"];

const auto v4Addr = ipSettings["v4"].get<std::string>();
const auto tcpPort = ipSettings["tcpPort"].get<uint16_t>();

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
    // Create IPv4 TCP socket
    ClientSocketIP sock{ { ConnectionType::TCP, "", v4Addr, tcpPort } };

    // Connect
    runSync([&sock]() -> Task<> {
        co_await sock.connect();
    });

    // Create a separate thread to briefly wait, then cancel I/O while recv() is pending
    std::thread readThread{ [&sock] {
        using namespace std::literals;

        std::this_thread::sleep_for(20ms);
        sock.cancelIO();
    } };

    // Start a receive operation
    // It should be interrupted by the second thread and throw an exception
    auto recvOperation = [&sock]() -> Task<> {
        co_await sock.recv();
    };

    CHECK_THROWS_MATCHES(runSync(recvOperation), System::SystemError, CancellationMatcher{});

    readThread.join();
}
