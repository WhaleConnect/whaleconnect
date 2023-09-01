// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Enumeration to determine socket types at compile time.
enum class SocketTag { IP, BT };

// All possible connection types.
// L2CAP connections are not supported on Windows because of limitations with the Microsoft Bluetooth stack.
enum class ConnectionType { TCP, UDP, L2CAP, RFCOMM, None };

// IP versions.
enum class IPType { None, V4, V6 };
