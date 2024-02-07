// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

import external.std;
import os.async;

// Listener to initialize OS APIs when tests are run.
struct InitListener : Catch::EventListenerBase {
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(const Catch::TestRunInfo&) override {
        // Putting the instance inside a static optional will give it a lifetime until the program ends, but
        // construction only when this function is called
        static std::optional<Async::Instance> asyncInstance;

        asyncInstance.emplace(1, 128);
    }
};

CATCH_REGISTER_LISTENER(InitListener)
