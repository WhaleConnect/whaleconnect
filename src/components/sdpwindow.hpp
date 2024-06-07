// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <future>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>

#include "window.hpp"
#include "windowlist.hpp"
#include "net/btutils.hpp"
#include "net/device.hpp"
#include "net/enums.hpp"
#include "os/error.hpp"

// Handles an SDP inquiry in a GUI window.
class SDPWindow : public Window {
    using AsyncSDPInquiry = std::future<std::vector<BTUtils::SDPResult>>; // Results of an SDP search

    Device target; // Target to perform SDP inquiries on and connect to

    // Fields for SDP connections
    std::size_t selectedUUID = 0; // UUID selected for an inquiry
    bool flushCache = false; // If data should be flushed on the next inquiry
    std::string serviceName; // Service name of the selected SDP result, displayed in the connection window title

    // Fields for SDP and manual connection state
    ConnectionType connType = ConnectionType::RFCOMM; // Selected connection type
    std::uint16_t connPort = 0; // Port to connect to

    // Fields for connection management
    WindowList& list; // List of connection window objects to add to when making a new connection

    // SDP inquiry data
    // The value this variant currently holds contains data about the SDP inquiry.
    // The type of the value represents the inquiry's state.
    std::variant<std::monostate, // Default value when no inquiries have been run yet
        AsyncSDPInquiry, // Future object corresponding to an in-progress inquiry
        std::system_error, // Error when the asynchronous thread couldn't be created
        System::SystemError, // Error that occurred during an in-progress inquiry
        std::vector<BTUtils::SDPResult> // The results of the inquiry when it has completed
        >
        sdpInquiry;

    // Draws the entries from an SDP lookup with buttons to connect to each in a tree format.
    bool drawSDPList(const std::vector<BTUtils::SDPResult>& list);

    // Draws the options for connecting to a device with Bluetooth.
    void drawConnOptions(std::string_view info);

    // Draws information about the SDP inquiry.
    void checkInquiryStatus();

    // Draws the tab to initiate an SDP inquiry.
    void drawSDPTab();

    // Draws the tab to initiate a connection without SDP.
    void drawManualTab();

    // Checks the status of the inquiry and prevents closing the window if it is running.
    void onBeforeUpdate() override;

    // Draws the window contents.
    void onUpdate() override;

public:
    // Sets the information needed to create connections.
    SDPWindow(std::string_view title, const Device& target, WindowList& list) :
        Window(title), target(target), list(list) {}
};
