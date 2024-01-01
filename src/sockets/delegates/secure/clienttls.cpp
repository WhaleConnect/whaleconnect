// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <chrono>
#include <coroutine> // IWYU pragma: keep
#include <cstdint>
#include <optional>
#include <span>
#include <string>

#include <botan/certstor_system.h>
#include <botan/tls_alert.h>
#include <botan/tls_client.h>
#include <botan/tls_policy.h>
#include <botan/tls_server_info.h>
#include <botan/tls_session_manager_memory.h>
#include <botan/system_rng.h>
#include <botan/x509cert.h>

module sockets.delegates.secure.clienttls;
import os.async;
import os.error;
import utils.overload;

class CredentialsManager : public Botan::Credentials_Manager {
    Botan::System_Certificate_Store systemCertStore;

public:
    std::vector<Botan::Certificate_Store*> trusted_certificate_authorities(const std::string&,
        const std::string&) override {
        return { &systemCertStore };
    }
};

class TLSCallbacks : public Botan::TLS::Callbacks {
    Delegates::ClientTLS& io;

public:
    explicit TLSCallbacks(Delegates::ClientTLS& io) : io(io) {}

    // The TLS channel is used as an adapter that takes unencrypted data and outputs encrypted data into a send queue
    void tls_emit_data(std::span<const uint8_t> buf) override {
        io.queueWrite({ reinterpret_cast<const char*>(buf.data()), buf.size() });
    }

    void tls_record_received(uint64_t, std::span<const uint8_t> buf) override {
        io.queueRead({ reinterpret_cast<const char*>(buf.data()), buf.size() });
    }

    void tls_alert(Botan::TLS::Alert alert) override {
        io.setAlert(alert);
        if (alert.is_fatal()) io.close(); // Fatal alerts deactivate connections
    }

    std::chrono::milliseconds tls_verify_cert_chain_ocsp_timeout() const override {
        using namespace std::literals;
        return 2000ms;
    }
};

Task<> Delegates::ClientTLS::sendQueued() {
    // Send encrypted data until queue is empty
    while (!pendingWrites.empty()) {
        std::string data = pendingWrites.front();
        pendingWrites.pop();
        co_await baseIO.send(data);
    }
}

Task<bool> Delegates::ClientTLS::recvBase(size_t size) {
    auto recvResult = co_await baseIO.recv(size);

    if (recvResult.closed) channel->close();
    else channel->received_data(reinterpret_cast<uint8_t*>(recvResult.data.data()), recvResult.data.size());

    co_return recvResult.closed;
}

void Delegates::ClientTLS::close() {
    if (channel && channel->is_active()) channel->close(); // Send close message to peer (must be before closing handle)

    handle.close();
}

Task<> Delegates::ClientTLS::connect(Device device) {
    co_await baseClient.connect(device);

    const auto rng = std::make_shared<Botan::System_RNG>();
    channel = std::make_unique<Botan::TLS::Client>(std::make_shared<TLSCallbacks>(*this),
        std::make_shared<Botan::TLS::Session_Manager_In_Memory>(rng), std::make_shared<CredentialsManager>(),
        std::make_shared<Botan::TLS::Policy>(), rng, Botan::TLS::Server_Information{ device.address, device.port });

    // Perform TLS handshake until channel is active
    do {
        // Client initiates handshake to server; send before receiving
        co_await sendQueued();
        co_await recvBase(1024);
    } while (!channel->is_active() && !channel->is_closed());
}

Task<> Delegates::ClientTLS::send(std::string data) {
    if (channel) {
        channel->send(data);
        co_await sendQueued();
    }
}

Task<RecvResult> Delegates::ClientTLS::recv(size_t size) {
    // A record may take multiple receive calls to come in
    if (completedReads.empty()) {
        if (co_await recvBase(size)) co_return { true, true, "", std::nullopt };

        co_return { false, false, "", std::nullopt };
    }

    auto queuedData = completedReads.front();
    completedReads.pop();
    co_return queuedData;
}
