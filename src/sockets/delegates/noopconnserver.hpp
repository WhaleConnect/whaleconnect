// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sockets/delegates.hpp"

namespace Delegates {
    // Provides no-ops for connection-oriented server operations.
    struct NoopConnServer : public ConnServerDelegate {
        Task<SocketPtr> accept() override {
            co_return nullptr;
        }
    };
}
