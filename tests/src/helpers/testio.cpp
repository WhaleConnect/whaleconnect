// Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "helpers.hpp"
#include "net/device.hpp"
#include "sockets/socket.hpp"
#include "utils/task.hpp"

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
            CHECK(recvResult.data == echoString);
        },
        useRunLoop);
}

void testIOClient(const Socket& socket, const Device& device, bool useRunLoop) {
    runSync([&socket, &device]() -> Task<> { co_await socket.connect(device); }, useRunLoop);

    testIO(socket, useRunLoop);
}
