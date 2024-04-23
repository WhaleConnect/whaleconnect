// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>
#include <string_view>

#include <botan/tls_exceptn.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

#include "helpers/helpers.hpp"
#include "net/enums.hpp"
#include "sockets/clientsockettls.hpp"
#include "sockets/delegates/delegates.hpp"
#include "utils/task.hpp"

// Matcher to check for specific TLS error cases.
struct TLSErrorMatcher : Catch::Matchers::MatcherGenericBase {
    std::string expectedError;

    TLSErrorMatcher(std::string_view expectedError) : expectedError(expectedError) {}

    bool match(const Botan::TLS::TLS_Exception& e) const {
        return e.what() == expectedError;
    }

    std::string describe() const override {
        return "Is a TLS error";
    }
};

TEST_CASE("HTTPS") {
    // Security check with howsmyssl.com
    SECTION("TLS check") {
        runSync([]() -> Task<> {
            ClientSocketTLS sock;
            co_await sock.connect({ ConnectionType::TCP, "", "www.howsmyssl.com", 443 });

            // Send HTTP API request
            co_await sock.send("GET /a/check HTTP/1.1\r\nHost: www.howsmyssl.com\r\nConnection: close\r\n\r\n");

            // Read response until socket closed
            std::string response;
            while (true) {
                RecvResult result = co_await sock.recv(1024);
                if (result.complete) response += result.data;

                if (!result.alert) continue;
                CHECK(result.alert->desc == "close_notify");

                // Socket closure should immediately follow close alert
                bool closed = (co_await sock.recv(1024)).closed;
                CHECK(closed);
                break;
            }

            // Check HTTP response
            CHECK(response.starts_with("HTTP/1.1 200 OK"));

            // Check attributes
            CHECK(response.contains("\"ephemeral_keys_supported\":true"));
            CHECK(response.contains("\"session_ticket_supported\":true"));
            CHECK(response.contains("\"insecure_cipher_suites\":{}"));
            CHECK(response.contains("\"tls_version\":\"TLS 1.3\""));
            CHECK(response.contains("\"rating\":\"Probably Okay\""));
        });
    }

    // Error handling checks with badssl.com

    SECTION("Self-signed certificate") {
        auto operation = []() -> Task<> {
            ClientSocketTLS sock;
            co_await sock.connect({ ConnectionType::TCP, "", "self-signed.badssl.com", 443 });
        };

        std::string expectedError = "Certificate validation failure: Cannot establish trust";
        CHECK_THROWS_MATCHES(runSync(operation), Botan::TLS::TLS_Exception, TLSErrorMatcher{ expectedError });
    }

    SECTION("Expired certificate") {
        auto operation = []() -> Task<> {
            ClientSocketTLS sock;
            co_await sock.connect({ ConnectionType::TCP, "", "expired.badssl.com", 443 });
        };

        std::string expectedError = "Certificate validation failure: Certificate has expired";
        CHECK_THROWS_MATCHES(runSync(operation), Botan::TLS::TLS_Exception, TLSErrorMatcher{ expectedError });
    }

    SECTION("Handshake failure") {
        runSync([]() -> Task<> {
            ClientSocketTLS sock;
            co_await sock.connect({ ConnectionType::TCP, "", "rc4.badssl.com", 443 });
            auto alert = *(co_await sock.recv(1024)).alert; // No data is actually being received
            CHECK(alert.isFatal);
            CHECK(alert.desc == "handshake_failure");
        });
    }
}
