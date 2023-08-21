// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sockets/delegates.hpp"
#include "sockets/device.hpp"
#include "utils/task.hpp"

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

    // Provides no-ops for connection-oriented server operations.
    struct NoopConnServer : public ConnServerDelegate {
        Task<AcceptResult> accept() override {
            co_return {};
        }
    };

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
