// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sockets/delegates.hpp"
#include "sockets/device.hpp"
#include "utils/task.hpp"

namespace Delegates {
    // Provides no-ops for client operations.
    class NoopClient : public ClientDelegate {
        static constexpr Device _nullDevice {};

    public:
        Task<> connect() override {
            co_return;
        }

        const Device& getServer() override {
            return _nullDevice;
        }
    };
}
