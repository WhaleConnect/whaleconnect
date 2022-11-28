// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connwindow.hpp"

#include <format>

#include <magic_enum.hpp>

#include "os/error.hpp"
#include "sockets/device.hpp"
#include "sockets/traits.hpp"
#include "utils/strings.hpp"

template <>
[[maybe_unused]] constexpr magic_enum::customize::customize_t
magic_enum::customize::enum_name(ConnectionType value) noexcept {
    using enum ConnectionType;

    switch (value) {
    case L2CAPSeqPacket: return "L2CAP SeqPacket";
    case L2CAPStream: return "L2CAP Stream";
    case L2CAPDgram: return "L2CAP Datagram";
    default: return default_tag;
    }
}

// Formats a Device instance into a string for use in a ConnWindow title.
std::string formatDevice(const Device& device, std::string_view extraInfo) {
    // Type of the connection
    bool isIP = (device.type == ConnectionType::TCP || device.type == ConnectionType::UDP);
    auto typeString = magic_enum::enum_name(device.type);

    // Bluetooth-based connections are described using the device's name (e.g. "MyESP32"),
    // IP-based connections use the device's IP address (e.g. 192.168.0.178).
    std::string deviceString = isIP ? device.address : device.name;

    // Newlines may be present in a Bluetooth device name, and if they get into a window's title, anything after the
    // first one will get cut off (the title bar can only hold one line). Replace them with enter characters (U+23CE)
    // to keep everything on one line.
    deviceString = Strings::replaceAll(deviceString, "\n", "\u23CE");

    // Format the values into a string as the title
    // The address is always part of the id hash.
    // The port is not visible for a Bluetooth connection, instead, it is part of the id hash.
    std::string title
        = isIP ? std::format("{} Connection - {} port {}##{}", typeString, deviceString, device.port, device.address)
               : std::format("{} Connection - {}##{} port {}", typeString, deviceString, device.address, device.port);

    // If there's extra info, it is formatted before the window title.
    // If it were to be put after the title, it would be part of the invisible id hash (after the "##").
    return extraInfo.empty() ? title : std::format("({}) {}", extraInfo, title);
}

ConnWindow::ConnWindow(std::unique_ptr<Writable>&& socket, const Device& device, std::string_view extraInfo) :
    Window(formatDevice(device, extraInfo)), _socket(std::move(socket)) {}

Task<> ConnWindow::_connect() try {
    // Connect the socket explicitly if it is an instance of the Connectable interface
    if (auto clientSocket = dynamic_cast<Connectable*>(_socket.get())) {
        _output.addInfo("Connecting...");
        co_await clientSocket->connect();
    }

    // Assume the socket is connected at this point
    _output.addInfo("Connected.");
} catch (const System::SystemError& error) { _errorHandler(error); }

Task<> ConnWindow::_sendHandler(std::string_view s) try {
    co_await _socket->send(std::string{ s });
} catch (const System::SystemError& error) { _errorHandler(error); }

Task<> ConnWindow::_readHandler() try {
    if (!_socket) co_return;
    if (_pendingRecv) co_return;

    _pendingRecv = true;
    std::string recvRet = co_await _socket->recv();

    if (recvRet.empty()) {
        // Peer closed connection (here, 0 does not necessarily mean success, i.e. NO_ERROR)
        _output.addInfo("Remote host closed connection.");
        _socket->close();
    } else {
        _output.addText(recvRet);
    }

    _pendingRecv = false;
} catch (const System::SystemError& error) {
    // Don't handle errors caused by socket closure (this means this object has been destructed)
#if OS_WINDOWS
    if (error.code == WSA_OPERATION_ABORTED) co_return;
#elif OS_LINUX
    if (error.code == ECANCELED) co_return;
#endif

    std::scoped_lock outputLock{ _outputMutex };
    _errorHandler(error);
}

void ConnWindow::_beforeUpdate() {
    ImGui::SetNextWindowSize({ 500, 300 }, ImGuiCond_FirstUseEver);
    _readHandler();
}

void ConnWindow::_updateContents() {
    std::scoped_lock outputLock{ _outputMutex };
    _output.update();
}
