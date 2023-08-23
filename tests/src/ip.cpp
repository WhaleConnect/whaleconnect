// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "helpers/helpers.hpp"
#include "helpers/testio.hpp"
#include "net/device.hpp"
#include "net/enums.hpp"
#include "os/async.hpp"
#include "os/error.hpp"
#include "sockets/clientsocket.hpp"

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
            ClientSocketIP s{ { TCP, "", v4Addr, tcpPort } };
            testIO(s);
        }

        SECTION("IPv6 TCP sockets") {
            ClientSocketIP s{ { TCP, "", v6Addr, tcpPort } };
            testIO(s);
        }
    }

    SECTION("UDP") {
        SECTION("IPv4 UDP sockets") {
            ClientSocketIP s{ { UDP, "", v4Addr, udpPort } };
            testIO(s);
        }

        SECTION("IPv6 UDP sockets") {
            ClientSocketIP s{ { UDP, "", v6Addr, udpPort } };
            testIO(s);
        }
    }
}
