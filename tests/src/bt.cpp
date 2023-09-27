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
const auto btSettings = settings["bluetooth"];

const auto mac = btSettings["mac"].get<std::string>();
const auto rfcommPort = btSettings["rfcommPort"].get<uint16_t>();

// L2CAP sockets are not supported on Windows
#if !OS_WINDOWS
const auto l2capPSM = btSettings["l2capPSM"].get<uint16_t>();
#endif

TEST_CASE("I/O (Bluetooth)") {
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
