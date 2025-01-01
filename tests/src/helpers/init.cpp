// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include "os/async.hpp"

// Listener to initialize OS APIs when tests are run.
struct InitListener : Catch::EventListenerBase {
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(const Catch::TestRunInfo&) override {
        Async::init(1, 128);
    }

    void testRunEnded(const Catch::TestRunStats&) override {
        Async::cleanup();
    }
};

CATCH_REGISTER_LISTENER(InitListener)
