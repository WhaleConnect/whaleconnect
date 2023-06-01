// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <optional>

#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include "os/async.hpp"

// Listener to initialize OS APIs when tests are run.
class InitListener : public Catch::EventListenerBase {
public:
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(Catch::TestRunInfo const&) override {
        // Putting the instance inside a static optional will give it a lifetime until the program ends, but
        // construction only when this function is called
        static std::optional<Async::Instance> asyncInstance;

        asyncInstance.emplace(1);
    }
};

CATCH_REGISTER_LISTENER(InitListener)
