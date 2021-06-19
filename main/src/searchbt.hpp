// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>

#include "sockets.hpp"

struct DeviceData;

namespace Sockets {
	/// <summary>
	/// A structure containing the results of a Bluetooth search.
	/// If the search failed, the last error can be found from the `err` member.
	/// If the search succeeded, a list of the devices discovered can be found from the `foundDevices` member.
	/// </summary>
	struct BTSearchResult {
		SocketError err;
		std::vector<DeviceData> foundDevices;
	};

    /// <summary>
	/// Search for nearby Bluetooth devices to connect to.
	/// </summary>
	/// <returns>Failure: last error and empty vector, Success: A DeviceData instance for each device found</returns>
	BTSearchResult searchBT();
}
