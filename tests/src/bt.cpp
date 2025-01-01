// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>

#include <catch2/catch_test_macros.hpp>

#include "helpers/testio.hpp"
#include "net/enums.hpp"
#include "sockets/clientsocket.hpp"
#include "utils/settingsparser.hpp"

TEST_CASE("I/O (Bluetooth)") {
    SettingsParser parser;
    parser.load(SETTINGS_FILE);

    const auto mac = parser.get<std::string>("bluetooth", "mac");
    const auto rfcommPort = parser.get<std::uint16_t>("bluetooth", "rfcommPort");

    // L2CAP sockets are not supported on Windows
#if !OS_WINDOWS
    const auto l2capPSM = parser.get<std::uint16_t>("bluetooth", "l2capPSM");
#endif

    using enum ConnectionType;

    SECTION("RFCOMM") {
        ClientSocketBT s;
        testIOClient(s, { RFCOMM, "", mac, rfcommPort }, true);
    }

#if !OS_WINDOWS
    SECTION("L2CAP") {
        ClientSocketBT s;
        testIOClient(s, { L2CAP, "", mac, l2capPSM }, true);
    }
#endif
}
