// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Bluetooth utility functions for Windows and Linux

#pragma once

#include <vector> // std::vector

#include "sockets.hpp"

/// <summary>
/// Namespace containing various Bluetooth utilities.
/// </summary>
namespace BTUtils {
#ifndef _WIN32
    /// <summary>
    /// Initialize GDBus and attempt to connect to bluetoothd (the Linux Bluetooth daemon) via DBus.
    /// </summary>
    void glibInit();

    /// <summary>
    /// Shut down the GDBus connection.
    /// </summary>
    void glibStop();

    // A message to display while waiting to connect to bluetoothd
    inline const char* glibDisconnectedMessage = "Waiting to connect to bluetoothd";
#endif
    /// <summary>
    /// Check if Bluetooth is initialized.
    /// </summary>
    /// <returns>
    /// On Linux: If the app has a DBus connection to bluetoothd, on Windows: true (no init on Windows)
    /// </returns>
    bool initialized();

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
    int getPaired(std::vector<Sockets::DeviceData>& deviceList);

    /// <summary>
    /// Get the channel of a Bluetooth server via Service Discovery Protocol (SDP).
    /// </summary>
    /// <param name="addr">The MAC address of the device</param>
    /// <returns>The channel number on success or 0 on failure</returns>
    uint8_t getSDPChannel(const char* addr);
}
