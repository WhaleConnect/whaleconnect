// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector> // std::vector

#include "util.hpp"

/// <summary>
/// Namespace containing various Bluetooth utilities.
/// </summary>
namespace BTUtil {
#ifndef _WIN32
    /// <summary>
    /// Initialize GDBus and attempt to connect to bluetoothd (the Linux Bluetooth daemon) via DBus.
    /// </summary>
    void glibInit();

    /// <summary>
    /// Shut down the GDBus connection.
    /// </summary>
    void glibStop();

    // If the app is connected to bluetoothd
    inline bool glibConnected = false;

    // A message to display while waiting to connect to bluetoothd
    inline const char* glibDisconnectedMessage = "Waiting to connect to bluetoothd...";
#endif

    /// <summary>
    /// Poll GLib events in a non-blocking manner on Linux. Do nothing on Windows.
    /// </summary>
    /// <remarks>
    /// This function is used to make GLib work without using its main loop. Instead, this function is called at the
    /// start of the application's main loop.
    /// </remarks>
    void glibMainContextIteration();

    /// <summary>
    /// Get the Bluetooth devices that are paired to this computer.
    /// </summary>
    /// <param name="deviceList">The vector that will be populated with the paired devices</param>
    /// <returns>NO_ERROR on success, SOCKET_ERROR on failure</returns>
    int getPaired(std::vector<DeviceData>& deviceList);
}
