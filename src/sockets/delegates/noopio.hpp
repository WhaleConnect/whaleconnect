// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sockets/delegates.hpp"

namespace Delegates {
    // Provides no-ops for I/O operations.
    struct NoopIO : public IODelegate {
        Task<> send(std::string) override {
            co_return;
        }

        Task<RecvResult> recv() override {
            co_return {};
        }

        void cancelIO() override {
            // No-op
        }
    };
}
