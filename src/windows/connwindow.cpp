// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connwindow.hpp"

#include <format>

#include <magic_enum.hpp>

#include "imgui.h"

#include "gui/imguiext.hpp"
#include "net/device.hpp"
#include "net/enums.hpp"
#include "os/error.hpp"
#include "utils/strings.hpp"

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name(ConnectionType value) noexcept {
    using enum ConnectionType;

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

// Formats a Device instance into a string for use in a ConnWindow title.
static std::string formatDevice(const Device& device, std::string_view extraInfo) {
    // Type of the connection
    bool isIP = (device.type == ConnectionType::TCP || device.type == ConnectionType::UDP);
    auto typeString = magic_enum::enum_name(device.type);

    // Bluetooth-based connections are described using the device's name (e.g. "MyESP32"),
    // IP-based connections use the device's IP address (e.g. 192.168.0.178).
    std::string deviceString = isIP ? device.address : device.name;

    // Newlines may be present in a Bluetooth device name, and if they get into a window's title, anything after the
    // first one will get cut off (the title bar can only hold one line). Replace them with left/down arrow icons
    // to keep everything on one line.
    deviceString = Strings::replaceAll(deviceString, "\n", "\uf306");

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

ConnWindow::ConnWindow(std::unique_ptr<Socket>&& socket, const Device& device, std::string_view extraInfo) :
    Window(formatDevice(device, extraInfo)), _socket(std::move(socket)) {}

Task<> ConnWindow::_connect() try {
    // Connect the socket
    _output.addInfo("Connecting...");
    co_await _socket->connect();

    _output.addInfo("Connected.");
    _connected = true;
} catch (const System::SystemError& error) {
    _errorHandler(error);
}

Task<> ConnWindow::_sendHandler(std::string_view s) try {
    co_await _socket->send(std::string{ s });
} catch (const System::SystemError& error) {
    _errorHandler(error);
}

Task<> ConnWindow::_readHandler() try {
    if (!_connected || _pendingRecv) co_return;

    _pendingRecv = true;
    auto recvRet = co_await _socket->recv();

    if (recvRet) {
        _output.addText(*recvRet);
    } else {
        // Peer closed connection
        _output.addInfo("Remote host closed connection.");
        _socket->close();
        _connected = false;
    }

    _pendingRecv = false;
} catch (const System::SystemError& error) {
    _errorHandler(error);
}

void ConnWindow::_errorHandler(const System::SystemError& error) {
    // Check for non-fatal errors, then add error line to console
    // Don't handle errors caused by I/O cancellation
    if (error && !error.isCanceled()) _output.addError(error.what());
}

void ConnWindow::_onBeforeUpdate() {
    using namespace ImGui::Literals;

    ImGui::SetNextWindowSize(35_fh * 20_fh, ImGuiCond_Appearing);
    _readHandler();
}

void ConnWindow::_onUpdate() {
    // Apply foxus to textbox
    // An InputTextMultiline is an InputText contained within a child window so focus must be set before rendering it to
    // apply focus to the InputText.
    if (_focusOnTextbox) {
        ImGui::SetKeyboardFocusHere();
        _focusOnTextbox = false;
    }

    // Textbox
    using namespace ImGui::Literals;

    float textboxHeight = 4_fh; // Number of lines that can be displayed
    ImVec2 size{ ImGui::FILL, textboxHeight };
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CtrlEnterForNewLine | ImGuiInputTextFlags_EnterReturnsTrue
                              | ImGuiInputTextFlags_AllowTabInput;

    if (ImGui::InputTextMultiline("##input", _textBuf, size, flags)) {
        // Line ending
        std::array endings{ "\n", "\r", "\r\n" };
        auto selectedEnding = endings[_currentLE];

        // InputTextMultiline() always uses \n as a line ending, replace all occurences of \n with the selected ending
        std::string sendString = Strings::replaceAll(_textBuf, "\n", selectedEnding);

        // Add a final line ending if set
        if (_addFinalLineEnding) sendString += selectedEnding;

        // Invoke the callback function if the string is not empty
        if (!sendString.empty()) {
            if (_sendEchoing) _output.addMessage(sendString, "SENT ", { 0.28f, 0.67f, 0.68f, 1 });

            _sendHandler(sendString);
        }

        // Blank out input textbox
        if (_clearTextboxOnSubmit) _textBuf.clear();

        _focusOnTextbox = true;
    }

    _output.update("console", ImVec2{ ImGui::FILL, -ImGui::GetFrameHeightWithSpacing() });
    _drawControls();
}

void ConnWindow::_drawControls() {
    // "Clear output" button
    if (ImGui::Button("Clear output")) _output.clear();

    // "Options" button
    ImGui::SameLine();
    if (ImGui::Button("Options...")) ImGui::OpenPopup("options");

    // Popup for more options
    if (ImGui::BeginPopup("options")) {
        _output.drawOptions();

        // Options for the input textbox
        ImGui::Separator();
        ImGui::MenuItem("Send echoing", nullptr, &_sendEchoing);
        ImGui::MenuItem("Clear texbox on send", nullptr, &_clearTextboxOnSubmit);
        ImGui::MenuItem("Add final line ending", nullptr, &_addFinalLineEnding);
        ImGui::EndPopup();
    }

    // Line ending combobox
    // The code used to calculate where to put the combobox is derived from
    // https://github.com/ocornut/imgui/issues/4157#issuecomment-843197490
    using namespace ImGui::Literals;

    float comboWidth = 10_fh;
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - comboWidth));
    ImGui::SetNextItemWidth(comboWidth);
    ImGui::Combo("##lineEnding", &_currentLE, "Newline\0Carriage return\0Both\0");
}
