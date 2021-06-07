// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>
#include <utility>

#include "util.hpp"

namespace Sockets {
    /// <summary>
	/// Search for nearby Bluetooth devices to connect to.
	/// </summary>
	/// <returns>Failure: last error code/empty vector, Success: A DeviceData instance for each device found</returns>
    std::pair<int, std::vector<DeviceData>> searchBluetoothDevices();
}
