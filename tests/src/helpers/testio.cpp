// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <coroutine> // IWYU pragma: keep

#include <catch2/catch_test_macros.hpp>

module helpers.testio;
import helpers.helpers;
import utils.task;

void testIO(const Socket& socket, bool useRunLoop) {
    // Check the socket is valid
    REQUIRE(socket.isValid());

    // Send/receive
    runSync(
        [&socket]() -> Task<> {
            constexpr const char* echoString = "echo test";

            co_await socket.send(echoString);

            // Receive data and check if the string matches
            // The co_await is outside the CHECK() macro to prevent it from being expanded and evaluated multiple times.
            auto recvResult = co_await socket.recv(1024);
            CHECK(recvResult == echoString);
        },
        useRunLoop);
}

void testIOClient(const Socket& socket, const Device& device, bool useRunLoop) {
    runSync([&socket, &device]() -> Task<> { co_await socket.connect(device); });

    testIO(socket, useRunLoop);
}
