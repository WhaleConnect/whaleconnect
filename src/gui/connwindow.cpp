// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <magic_enum.hpp>

#include "connwindow.hpp"
#include "app/settings.hpp"
#include "net/sockets.hpp"
#include "sys/error.hpp"
#include "compat/format.hpp"
#include "util/strings.hpp"

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name(Sockets::ConnectionType value) noexcept {
    using enum Sockets::ConnectionType;

    switch (value) {
    case L2CAPSeqPacket:
        return "L2CAP SeqPacket";
    case L2CAPStream:
        return "L2CAP Stream";
    case L2CAPDgram:
        return "L2CAP Datagram";
    default:
        return default_tag;
    }
}

// Formats a DeviceData instance into a string for use in a ConnWindow title.
static std::string formatDeviceData(const Sockets::DeviceData& data, std::string_view extraInfo) {
    // Type of the connection
    bool isBluetooth = Sockets::connectionTypeIsBT(data.type);
    auto typeString = magic_enum::enum_name(data.type);

    // Bluetooth connections are described using the device's name (e.g. "MyESP32"),
    // IP-based connections use the device's IP address (e.g. 192.168.0.178).
    std::string deviceString = isBluetooth ? data.name : data.address;

    // Newlines may be present in a Bluetooth device name, and if they get into a window's title, anything after the
    // first one will get cut off (the title bar can only hold one line). Replace them with enter characters (U+23CE)
    // to keep everything on one line.
    deviceString = Strings::replaceAll(deviceString, "\n", "\u23CE");

    // Format the values into a string as the title
    // The address is always part of the id hash.
    // The port is not visible for a Bluetooth connection, instead, it is part of the id hash.
    std::string title = isBluetooth
        ? std2::format("{} Connection - {}##{} port {}", typeString, deviceString, data.address, data.port)
        : std2::format("{} Connection - {} port {}##{}", typeString, deviceString, data.port, data.address);

    // If there's extra info, it is formatted before the window title.
    // If it were to be put after the title, it would be part of the invisible id hash (after the "##").
    return extraInfo.empty() ? title : std2::format("({}) {}", extraInfo, title);
}

ConnWindow::ConnWindow(const Sockets::DeviceData& data, std::string_view extraInfo)
    : Window(formatDeviceData(data, extraInfo), { 500, 300 }), _data(data) {}

void ConnWindow::_updateContents() {
    _readHandler();

    std::scoped_lock outputLock{ _outputMutex };
    _output.update(Settings::sendTextboxHeight);
}

Task<> ConnWindow::_connect() try {
    _output.addInfo("Connecting...");

    _socket = co_await Sockets::createClientSocket(_data);
    _output.addInfo("Connected.");
    _connected = true;
} catch (const System::SystemError& error) {
    _errorHandler(error);
}

Task<> ConnWindow::_sendHandler(std::string_view s) try {
    co_await Sockets::sendData(_socket.get(), s);
} catch (const System::SystemError& error) {
    _errorHandler(error);
}

Task<> ConnWindow::_readHandler() try {
    if (!_connected) co_return;
    if (_pendingRecv) co_return;

    _pendingRecv = true;
    const auto& recvRet = co_await Sockets::recvData(_socket.get());

    if (recvRet.bytesRead == 0) {
        // Peer closed connection (here, 0 does not necessarily mean success, i.e. NO_ERROR)
        _output.addInfo("Remote host closed connection.");
        _socket.reset();
        _connected = false;
    } else {
        _output.addText(recvRet.data);
    }

    _pendingRecv = false;
} catch (const System::SystemError& error) {
#ifdef _WIN32
    // Don't handle errors caused by socket closure (this means this object has been destructed)
    if (error.code == WSA_OPERATION_ABORTED) return;
#endif

    std::scoped_lock outputLock{ _outputMutex };
    _errorHandler(error);
}
