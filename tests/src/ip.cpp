// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

import helpers.helpers;
import helpers.testio;
import net.device;
import net.enums;
import os.async;
import os.error;
import sockets.clientsocket;

const auto settings = loadSettings();
const auto ipSettings = settings["ip"];

const auto v4Addr = ipSettings["v4"].get<std::string>();
const auto v6Addr = ipSettings["v4"].get<std::string>();
const auto tcpPort = ipSettings["tcpPort"].get<uint16_t>();
const auto udpPort = ipSettings["udpPort"].get<uint16_t>();

TEST_CASE("I/O (Internet Protocol)") {
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
