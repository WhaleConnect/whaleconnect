// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

import external.std;
import helpers.helpers;
import helpers.testio;
import net.device;
import net.enums;
import os.async;
import os.error;
import sockets.clientsocket;
import utils.settingsparser;

TEST_CASE("I/O (Bluetooth)") {
    SettingsParser parser;
    parser.load(SETTINGS_FILE);

    const auto mac = parser.get<std::string>("bluetooth", "mac");
    const auto rfcommPort = parser.get<u16>("bluetooth", "rfcommPort");

    // L2CAP sockets are not supported on Windows
#if !OS_WINDOWS
    const auto l2capPSM = parser.get<u16>("bluetooth", "l2capPSM");
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
