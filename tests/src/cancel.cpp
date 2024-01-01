// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <coroutine> // IWYU pragma: keep
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>
#include <nlohmann/json.hpp>

import helpers.helpers;
import net.enums;
import os.async;
import os.error;
import sockets.clientsocket;
import utils.task;

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
    const auto settings = loadSettings();
    const auto ipSettings = settings["ip"];

    const auto v4Addr = ipSettings["v4"].get<std::string>();
    const auto tcpPort = ipSettings["tcpPort"].get<uint16_t>();

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
