// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sockets/socket.hpp"

// Performs basic I/O checks on a socket.
void testIO(const Socket& socket, bool useRunLoop = false);
