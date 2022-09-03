// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief A class to handle an SDP inquiry in a GUI window
*/

#pragma once

#include <map>
#include <future>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>

#include "windowlist.hpp"
#include "window.hpp"
#include "net/sockets.hpp"
#include "net/btutils.hpp"
#include "sys/error.hpp"
#include "util/formatcompat.hpp"

class SDPWindow : public Window {
    using AsyncSDPInquiry = std::future<BTUtils::SDPResultList>;
    using UUIDMap = std::map<std::string, UUID, std::less<>>;

    Sockets::DeviceData _target;

    // Fields for SDP connections
    const UUIDMap& _uuids;
    std::string _selectedUUID;
    bool _flushCache = false;
    uint16_t _sdpPort = 0;
    std::string _serviceName;

    // Fields for manual connections
    Sockets::ConnectionType _connType = Sockets::ConnectionType::RFCOMM;
    uint16_t _port = 0;

    // Fields for connection window management
    WindowList& _list;
    bool _isNew = true;

    // SDP inquiry data
    std::variant<
        std::monostate, // Default variant value when no SDP inquiries have been run yet
        AsyncSDPInquiry, // A future object corresponding to an in-progress inquiry
        std::system_error, // A system error when the asynchronous thread couldn't be created
        System::SystemError, // A system error when an error occurred during the inquiry
        BTUtils::SDPResultList // The results of the inquiry when it has completed
    > _sdpInquiry;

    // Draws the options for connecting to a device with Bluetooth.
    bool _drawBTConnOptions();

    // Displays the entries from an SDP lookup with buttons to connect to each in a tree format.
    bool _drawSDPList(const BTUtils::SDPResultList& list);

    void _drawConnectionOptions(uint16_t port, std::string_view extraInfo);

    void _checkInquiryStatus();

    void _drawSDPOptions();

    void _drawManualOptions();

    void _updateContents() override;

public:
    SDPWindow(const Sockets::DeviceData& target, const UUIDMap& uuids, WindowList& list)
        : Window(std::format("Connect To {}##{}", target.name, target.address)), _target(target), _uuids(uuids),
        _selectedUUID(_uuids.begin()->first), _list(list) {}
};
