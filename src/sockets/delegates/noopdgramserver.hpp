// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sockets/delegates.hpp"

namespace Delegates {
    // Provides no-ops for datagram-oriented server operations.
    struct NoopDgramServer : public DgramServerDelegate {
        Task<DgramRecvResult> recvFrom() override {
            co_return {};
        }

        Task<> sendTo(std::string, std::string) override {
            co_return;
        }
    };
}
