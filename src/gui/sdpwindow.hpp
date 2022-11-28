// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <format>
#include <future>
#include <map>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>

#include "os/btutils.hpp"
#include "os/error.hpp"
#include "sockets/device.hpp"
#include "sockets/traits.hpp"
#include "window.hpp"
#include "windowlist.hpp"

// A class to handle an SDP inquiry in a GUI window.
class SDPWindow : public Window {
    using AsyncSDPInquiry = std::future<BTUtils::SDPResultList>;          // Results of an SDP search
    using UUIDMap = std::map<std::string, BTUtils::UUID128, std::less<>>; // List of UUIDs used for SDP filtering

    Device _target; // Target to perform SDP inquiries on and connect to

    // Fields for SDP connections
    const UUIDMap& _uuids;     // Available UUIDs used for SDP inquiries
    std::string _selectedUUID; // UUID selected for an inquiry
    bool _flushCache = false;  // If data should be flushed on the next inquiry
    std::string _serviceName;  // Service name of the selected SDP result, displayed in the connection window title

    // Fields for SDP and manual connection state
    ConnectionType _connType = ConnectionType::RFCOMM; // Selected connection type
    uint16_t _port = 0;                                // Port to connect to

    // Fields for connection management
    WindowList& _list; // List of connection window objects to add to when making a new connection

    // SDP inquiry data
    // The value this variant currently holds contains data about the SDP inquiry.
    // The type of the value represents the inquiry's state.
    std::variant<std::monostate,        // Default value when no inquiries have been run yet
                 AsyncSDPInquiry,       // A future object corresponding to an in-progress inquiry
                 std::system_error,     // An error when the asynchronous thread couldn't be created
                 System::SystemError,   // An error that occurred during an in-progress inquiry
                 BTUtils::SDPResultList // The results of the inquiry when it has completed
                 >
        _sdpInquiry;

    // Draws the entries from an SDP lookup with buttons to connect to each in a tree format.
    bool _drawSDPList(const BTUtils::SDPResultList& list);

    // Draws the options for connecting to a device with Bluetooth.
    void _drawConnOptions(std::string_view info);

    // Draws information about the SDP inquiry.
    void _checkInquiryStatus();

    // Draws the tab to initiate an SDP inquiry.
    void _drawSDPTab();

    // Draws the tab to initiate a connection without SDP.
    void _drawManualTab();

    // Checks the status of the inquiry and prevents closing the window if it is running.
    void _beforeUpdate() override;

    // Draws the window contents.
    void _updateContents() override;

public:
    // Sets the information needed to create connections.
    SDPWindow(const Device& target, const UUIDMap& uuids, WindowList& list) :
        Window(std::format("Connect To {}##{}", target.name, target.address)), _target(target), _uuids(uuids),
        _selectedUUID(_uuids.begin()->first), _list(list) {}
};
