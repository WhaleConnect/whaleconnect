// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <imgui.h>

#include "sdpwindow.hpp"
#include "connwindow.hpp"
#include "util/formatcompat.hpp"
#include "util/imguiext.hpp"
#include "util/overload.hpp"

// Draws the options for connecting to a device with Bluetooth.
static Sockets::ConnectionType drawBTConnOptions() {
    using enum Sockets::ConnectionType;

    static Sockets::ConnectionType type = RFCOMM;

    // Connection type selection
    if (ImGui::RadioButton("RFCOMM", type == RFCOMM)) type = RFCOMM;
    if (ImGui::RadioButton("L2CAP Sequential Packet", type == L2CAPSeqPacket)) type = L2CAPSeqPacket;
    if (ImGui::RadioButton("L2CAP Stream", type == L2CAPStream)) type = L2CAPStream;
    if (ImGui::RadioButton("L2CAP Datagram", type == L2CAPDgram)) type = L2CAPDgram;

    ImGui::Spacing();
    return ImGui::Button("Connect") ? type : None;
}

// Displays the entries from an SDP lookup with buttons to connect to each in a tree format.
static bool drawSDPList(const BTUtils::SDPResultList& list, uint16_t& portOut, std::string& serviceNameOut) {
    bool ret = false;

    // ID to use in case multiple services have the same name
    unsigned int id = 0;
    for (const auto& [protos, services, profiles, port, name, desc] : list) {
        const char* serviceName = name.empty() ? "Unnamed service" : name.c_str();

        ImGui::PushID(id); // Push the ID, then increment it
        id++; // TODO: Use views::enumerate() in C++26

        if (ImGui::TreeNode(serviceName)) {
            // Print the description (if there is one)
            ImGui::Text("Description: %s", desc.empty() ? "(none)" : desc.c_str());

            // Print UUIDs
            if (!protos.empty()) ImGui::Text("Protocol UUIDs:");
            for (auto i : protos) ImGui::BulletText("0x%04X", i);

            // Print service class UUIDs
            if (!services.empty()) ImGui::Text("Service class UUIDs:");
            for (const auto& i : services)
                ImGui::BulletText("%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                                  i.Data1, i.Data2, i.Data3,
                                  i.Data4[0], i.Data4[1], i.Data4[2], i.Data4[3],
                                  i.Data4[4], i.Data4[5], i.Data4[6], i.Data4[7]);

            // Print profile descriptors
            if (!profiles.empty()) ImGui::Text("Profile descriptors:");
            for (const auto& [uuid, verMajor, verMinor] : profiles)
                ImGui::BulletText("0x%04X (version %d.%d)", uuid, verMajor, verMinor);

            // Print the port
            ImGui::Text("Port: %d", port);

            // Connection options
            if (ImGui::Button("Connect...")) {
                serviceNameOut = serviceName;
                portOut = port;
                ret = true;
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    return ret;
}

void SDPWindow::_drawConnectionOptions(uint16_t port, std::string_view extraInfo) {
    auto type = drawBTConnOptions();
    if (type != Sockets::ConnectionType::None)
        _isNew = _list.add<ConnWindow>(Sockets::DeviceData{ type, _target.name, _target.address, port }, extraInfo);
}

void SDPWindow::_checkInquiryStatus() {
    std::visit(Overload{
        [](std::monostate) {
            ImGui::TextUnformatted("No inquiry run");
        },
        [this](AsyncSDPInquiry& asyncInq) {
            using namespace std::literals;
            if (asyncInq.wait_for(0s) != std::future_status::ready) {
                // Running, display a spinner
                ImGui::LoadingSpinner("Running SDP inquiry");
                return;
            }

            try {
                _sdpInquiry = asyncInq.get();
            } catch (const System::SystemError& error) {
                _sdpInquiry = error;
            }
        },
        [](const std::system_error&) {
            ImGui::TextWrapped("System error: Failed to launch thread.");
        },
        [](const System::SystemError& error) {
            ImGui::TextWrapped("Error %s", error.formatted().c_str());
        },
        [this](const BTUtils::SDPResultList& list) {
            // Done, print results
            if (list.empty()) {
                ImGui::Text("No SDP results found for \"%s\".", _target.name.c_str());
                return;
            }

            static uint16_t port = 0;
            static std::string serviceName;
            if (drawSDPList(list, port, serviceName)) ImGui::OpenPopup("options");
            if (ImGui::BeginPopup("options")) {
                _drawConnectionOptions(port, serviceName);
                ImGui::EndPopup();
            }
        }
    }, _sdpInquiry);
}

void SDPWindow::_drawSDPOptions() {
    ImGui::BeginDisabled(std::holds_alternative<AsyncSDPInquiry>(_sdpInquiry));

    // UUID selection combobox
    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::BeginCombo("Protocol/Service UUID", _selectedUUID.c_str())) {
        for (const auto& [name, _] : _uuids) if (ImGui::Selectable(name.c_str())) _selectedUUID = name;
        ImGui::EndCombo();
    }

#ifdef _WIN32
    // "Flush cache" option (Windows only)
    ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x * 4);
    ImGui::Checkbox("Flush cache", &_flushCache);
    ImGui::HelpMarker("Ignore previous cached advertising data on this inquiry.");
#endif

    if (ImGui::Button("Run SDP Inquiry")) {
        try {
            _sdpInquiry = std::async(std::launch::async, BTUtils::sdpLookup,
                                     _target.address, _uuids[_selectedUUID], _flushCache);
        } catch (const std::system_error& error) {
            _sdpInquiry = error;
        }
    }

    ImGui::EndDisabled();
    _checkInquiryStatus();
}

void SDPWindow::_drawManualOptions() {
    ImGui::Spacing();
    ImGui::SetNextItemWidth(100.0f);
    ImGui::InputScalar("Port", _port, 1, 10);

    _drawConnectionOptions(_port, std::format("Port {}", _port));
}

void SDPWindow::_updateContents() {
    if (ImGui::CollapsingHeader("Connect with SDP")) _drawSDPOptions();
    if (ImGui::CollapsingHeader("Connect Manually")) _drawManualOptions();

    // If the connection exists, show a message
    if (!_isNew) ImGui::Text("This connection is already open.");
}
