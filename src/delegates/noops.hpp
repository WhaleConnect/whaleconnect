// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "delegates.hpp"

#include "net/device.hpp"
#include "net/enums.hpp"
#include "utils/task.hpp"

namespace Delegates {
    // Provides no-ops for I/O operations.
    struct NoopIO : IODelegate {
        Task<> send(std::string) override {
            co_return;
        }

        Task<RecvResult> recv(size_t) override {
            co_return {};
        }
    };

    // Provides no-ops for client operations.
    struct NoopClient : ClientDelegate {
        Task<> connect(Device) override {
            co_return;
        }
    };

    // Provides no-ops for server operations.
    struct NoopServer : ServerDelegate {
        ServerAddress startServer(const Device&) override {
            return {};
        }

        Task<AcceptResult> accept() override {
            co_return {};
        }

        Task<DgramRecvResult> recvFrom(size_t) override {
            co_return {};
        }

        Task<> sendTo(Device, std::string) override {
            co_return;
        }
    };
}
