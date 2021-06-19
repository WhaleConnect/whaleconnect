// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#ifndef _WIN32
struct bdaddr_t; // This structure is defined in <bluetooth/bluetooth.h>, provide a forward declaration for it
#endif

// Get the length of an array
#define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))

// Enum of all possible connection types
enum ConnectionTypes { TCP, UDP, Bluetooth };
inline const char* connectionTypesStr[] = { "TCP", "UDP", "Bluetooth" }; // String representations of connection types

/// <summary>
/// Structure containing metadata about a device (type, name, address, port) to provide information on how to connect to
/// it and how to describe it.
/// </summary>
struct DeviceData {
	int type; // Type of connection
	std::string name; // Name of device (Bluetooth only)
	std::string address; // Address of device (IP address for TCP/UDP, MAC address for Bluetooth)
	uint16_t port; // Port/channel of device

	// Bluetooth address (platform-specific)
#ifdef _WIN32
	uint64_t btAddr; // Bluetooth address as a 64-bit unsigned integer
#else
	bdaddr_t btAddr; // Bluetooth address as a bdaddr_t structure
#endif
};

/// <summary>
/// Namespace containing mutable values to configure the application.
/// </summary>
namespace Settings {
	inline uint8_t fontSize = 13; // Application font height in pixels
	inline uint8_t maxRecents = 10; // Number of recent connection entries allowed
	inline uint8_t connectTimeout = 5; // Number of seconds to allow for connection before it aborts
	inline uint8_t btSearchTime = 5; // Duration of Bluetooth search in seconds
}
