// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "helpers/helpers.hpp"
#include "os/async.hpp"
#include "os/error.hpp"
#include "sockets/clientsocket.hpp"
#include "sockets/device.hpp"
#include "sockets/interfaces.hpp"
#include "sockets/traits.hpp"

const auto settings = loadSettings();
const auto ipSettings = settings["ip"];

const auto v4Addr = ipSettings["v4"].get<std::string>();
const auto v6Addr = ipSettings["v4"].get<std::string>();
const auto tcpPort = ipSettings["tcpPort"].get<uint16_t>();
const auto udpPort = ipSettings["udpPort"].get<uint16_t>();

// Creates a socket connected to the targe device and performs basic I/O checks.
void runTests(const Device& device) {
    auto sock = createClientSocket<SocketTag::IP>(device);

    // Connect
    runSync([&sock]() -> Task<> {
        co_await sock->connect();
    });

    // Check the socket is valid
    REQUIRE(sock->isValid());

    // Send/receive
    runSync([&sock]() -> Task<> {
        constexpr const char* echoString = "echo test";

        co_await sock->send(echoString);

        // Receive data and check if the string matches
        // The co_await is outside the CHECK() macro to prevent it from being expanded and evaluated multiple times.
        auto recvResult = co_await sock->recv();
        CHECK(recvResult == echoString);
    });

    // Close (socket should be invalidated as a postcondition)
    sock->close();
    CHECK(!sock->isValid());
}

TEST_CASE("I/O (Internet Protocol)") {
    using enum ConnectionType;

    SECTION("TCP") {
        SECTION("IPv4 TCP sockets") {
            runTests({ TCP, "", v4Addr, tcpPort });
        }

        SECTION("IPv6 TCP sockets") {
            runTests({ TCP, "", v6Addr, tcpPort });
        }
    }

    SECTION("UDP") {
        SECTION("IPv4 UDP sockets") {
            runTests({ UDP, "", v4Addr, udpPort });
        }

        SECTION("IPv6 UDP sockets") {
            runTests({ UDP, "", v6Addr, udpPort });
        }
    }
}
