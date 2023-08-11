// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "testio.hpp"

#include <catch2/catch_test_macros.hpp>

#include "helpers.hpp"

void testIO(const Socket& socket, bool useRunLoop) {
    // Connect
    runSync([&socket]() -> Task<> { co_await socket.connect(); }, useRunLoop);

    // Check the socket is valid
    REQUIRE(socket.isValid());

    // Send/receive
    runSync(
        [&socket]() -> Task<> {
            constexpr const char* echoString = "echo test";

            co_await socket.send(echoString);

            // Receive data and check if the string matches
            // The co_await is outside the CHECK() macro to prevent it from being expanded and evaluated multiple times.
            auto recvResult = co_await socket.recv();
            CHECK(recvResult == echoString);
        },
        useRunLoop);
}
