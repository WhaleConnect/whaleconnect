// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connwindowlist.hpp"
#include "util/formatcompat.hpp"

#ifndef _WIN32
constexpr auto WSAEINVAL = EINVAL;
constexpr auto WSAENOTSOCK = ENOTSOCK;
#endif

/// <summary>
/// Get the textual name of a ConnectionType.
/// </summary>
/// <param name="type">The ConnectionType</param>
/// <returns>The textual name as a const char*</returns>
/// <remarks>
/// Returning a const char* is safe for this function since there are no local variables that are being returned,
/// just string literals.
/// </remarks>
static constexpr const char* connTypeToStr(Sockets::ConnectionType type) {
    using enum Sockets::ConnectionType;

    switch (type) {
    case TCP:
        return "TCP";
    case UDP:
        return "UDP";
    case L2CAPSeqPacket:
        return "L2CAP SeqPacket";
    case L2CAPStream:
        return "L2CAP Stream";
    case L2CAPDgram:
        return "L2CAP Datagram";
    case RFCOMM:
        return "RFCOMM";
    case None:
        return "None";
    default:
        return "Unknown";
    }
}

/// <summary>
/// Format a DeviceData instance into a string for use in a ConnWindow title.
/// </summary>
/// <param name="data">The DeviceData to format</param>
/// <returns>The resultant string</returns>
static std::string formatDeviceData(const Sockets::DeviceData& data) {
    // Type of the connection
    Sockets::ConnectionType type = data.type;
    bool isBluetooth = (type != Sockets::ConnectionType::TCP) && (type != Sockets::ConnectionType::UDP);

    // Bluetooth connections are described using the device's name (e.g. "MyESP32"),
    // IP connections (TCP/UDP) use the device's IP address (e.g. 192.168.0.178).
    std::string deviceString = (isBluetooth) ? data.name : data.address;

    // Newlines may be present in a Bluetooth device name, and if they get into a window's title, anything after the
    // first one will get cut off (the title bar can only hold one line). Replace them with enter characters (U+23CE)
    // to keep everything on one line.
    deviceString = Strings::replaceAll(deviceString, "\n", "\u23CE");

    // Format the values into a string as the title
    // The address is always part of the id hash.
    // The port is not visible for a Bluetooth connection, instead, it is part of the id hash.
    const char* fmtStr = (isBluetooth) ? "{} Connection - {}##{} {}" : "{} Connection - {} port {}##{}";
    return std::format(fmtStr, connTypeToStr(type), deviceString, data.port, data.address);
}

void ConnWindowList::_populateFds() {
    // Clear the file descriptors vector, then populate it with the data in the `_windows` vector
    _pfds.clear();
    for (const auto& window : _windows) _pfds.emplace_back(window->getSocket(), window->getPollFlags(), 0);
}

bool ConnWindowList::add(const Sockets::DeviceData& data, const std::string& extraInfo) {
    // Title of the ConnWindow
    std::string title = formatDeviceData(data);

    // Check if the title already exists
    bool isNew = std::find_if(_windows.begin(), _windows.end(), [title](const ConnWindowPtr& current) {
        return current->titleEquals(title);
    }) == _windows.end();

    // Add the window to the list
    if (isNew) {
        SOCKET sockfd = _connectFunction(data); // Create the socket
        int lastErr = Sockets::getLastErr(); // Get the error immediately
        _windows.push_back(std::make_unique<ConnWindow>(title, extraInfo, sockfd, lastErr));
        _populateFds(); // Update the vector for polling
    }

    return isNew;
}

int ConnWindowList::update() {
    // Iterate through the windows vector
    for (size_t i = 0; i < _windows.size(); i++) {
        // The current window
        auto& current = _windows[i];

        if (current->open()) {
            // Window is open, update it
            current->update();
        } else {
            // Window is closed, remove it from vector
            _windows.erase(_windows.begin() + i);

            // Update file descriptor poll vector
            _populateFds();

            // Iterators are invalid, wait until the next iteration
            break;
        }
    }

    // Polling code starts below
    // If a previous poll failed, abort prematurely
    // NO_ERROR is returned to prevent duplicate error checking, i.e. the code checks for SOCKET_ERROR returned, so it
    // is only returned when the error actually occurs, not on subsequent operations.
    if (_pollRet == SOCKET_ERROR) return NO_ERROR;

    // Make sure that the two vectors of windows and sockets are equal in size
    assert(_windows.size() == _pfds.size());

    // Check all sockets for events
    _pollRet = Sockets::poll(_pfds, 0);
    if (_pollRet == SOCKET_ERROR) {
        // Get the last error if poll failed
        int lastErr = Sockets::getLastErr();

        // (WSA)EINVAL and (WSA)ENOTSOCK are non-fatal errors that indicate an operation occurred on an invalid socket.
        // These are thrown when a socket in the list disconnects or when the program starts up with nothing to poll.
        if ((lastErr == WSAEINVAL) || (lastErr == WSAENOTSOCK)) {
            _pollRet = NO_ERROR;
            Sockets::setLastErr(NO_ERROR);
        } else {
            return SOCKET_ERROR; // A fatal error occurred
        }
    }

    // Poll was successful, call the appropriate handlers on each window
    for (size_t i = 0; i < _pfds.size(); i++) {
        auto revents = _pfds[i].revents;
        auto& current = _windows[i];

        // A signal for input indicates the socket has data to read
        // A signal for hangup indicates the peer has disconnected
        // Both events are handled in `inputHandler()`.
        if ((revents & POLLIN) || (revents & POLLHUP)) current->inputHandler();

        // A signal for output indicates the socket has connected
        if (revents & POLLOUT) current->connectHandler();

        // Update events if they've changed
        _pfds[i].events = current->getPollFlags();
    }

    return NO_ERROR;
}
