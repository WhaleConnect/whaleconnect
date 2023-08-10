// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "helpers/helpers.hpp"
#include "helpers/testio.hpp"
#include "os/async.hpp"
#include "os/error.hpp"
#include "sockets/clientsocket.hpp"
#include "sockets/device.hpp"
#include "sockets/enums.hpp"

const auto settings = loadSettings();
const auto btSettings = settings["bluetooth"];

const auto mac = btSettings["mac"].get<std::string>();
const auto rfcommPort = btSettings["rfcommPort"].get<uint16_t>();
const auto l2capPSM = btSettings["l2capPSM"].get<uint16_t>();

TEST_CASE("I/O (Bluetooth)") {
    using enum ConnectionType;

    SECTION("RFCOMM") {
        ClientSocketBT s{ { RFCOMM, "", mac, rfcommPort } };
        testIO(s);
    }

#if !OS_WINDOWS
    SECTION("L2CAP") {
        ClientSocketBT s{ { L2CAPStream, "", mac, l2capPSM } };
        testIO(s);
    }
#endif
}
