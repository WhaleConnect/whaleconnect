// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>

#include <catch2/catch_test_macros.hpp>

#include "helpers/testio.hpp"
#include "net/enums.hpp"
#include "sockets/clientsocket.hpp"
#include "utils/settingsparser.hpp"

TEST_CASE("I/O (Internet Protocol)") {
    SettingsParser parser;
    parser.load(SETTINGS_FILE);

    const auto v4Addr = parser.get<std::string>("ip", "v4");
    const auto v6Addr = parser.get<std::string>("ip", "v6");
    const auto tcpPort = parser.get<std::uint16_t>("ip", "tcpPort");
    const auto udpPort = parser.get<std::uint16_t>("ip", "udpPort");

    using enum ConnectionType;

    SECTION("TCP") {
        SECTION("IPv4 TCP sockets") {
            ClientSocketIP s;
            testIOClient(s, { TCP, "", v4Addr, tcpPort });
        }

        SECTION("IPv6 TCP sockets") {
            ClientSocketIP s;
            testIOClient(s, { TCP, "", v6Addr, tcpPort });
        }
    }

    SECTION("UDP") {
        SECTION("IPv4 UDP sockets") {
            ClientSocketIP s;
            testIOClient(s, { UDP, "", v4Addr, udpPort });
        }

        SECTION("IPv6 UDP sockets") {
            ClientSocketIP s;
            testIOClient(s, { UDP, "", v6Addr, udpPort });
        }
    }
}
