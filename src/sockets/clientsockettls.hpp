// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "socket.hpp"

#include "delegates/noops.hpp"
#include "delegates/secure/clienttls.hpp"

// An outgoing connection secured by TLS.
class ClientSocketTLS : public Socket {
    Delegates::ClientTLS client;
    Delegates::NoopServer server;

public:
    ClientSocketTLS() : Socket(client, client, client, server) {}
};
