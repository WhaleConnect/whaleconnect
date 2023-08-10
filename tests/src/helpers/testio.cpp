// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "testio.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <catch2/catch_test_macros.hpp>

#include "helpers.hpp"

void testIO(const Socket& socket) {
    // Connect
    [&socket]() -> Task<> {
        co_await socket.connect();
        REQUIRE(socket.isValid());

        constexpr const char* echoString = "echo test";

        co_await socket.send(echoString);

        // Receive data and check if the string matches
        // The co_await is outside the CHECK() macro to prevent it from being expanded and evaluated multiple times.
        auto recvResult = co_await socket.recv();
        CHECK(recvResult == echoString);

        CFRunLoopStop(CFRunLoopGetCurrent());
    }();

    // Check the socket is valid

    // Send/receive
    // runSync([&socket]() -> Task<> {
    //     constexpr const char* echoString = "echo test";

    //     co_await socket.send(echoString);

    //     // Receive data and check if the string matches
    //     // The co_await is outside the CHECK() macro to prevent it from being expanded and evaluated multiple times.
    //     auto recvResult = co_await socket.recv();
    //     CHECK(recvResult == echoString);

    //     CFRunLoopStop(CFRunLoopGetCurrent());
    // });

    CFRunLoopRun();
}
