// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <imgui.h>

#include "sdpwindow.hpp"
#include "connwindow.hpp"
#include "util/formatcompat.hpp"
#include "util/imguiext.hpp"
#include "util/overload.hpp"

bool SDPWindow::_drawBTConnOptions() {
    using enum Sockets::ConnectionType;

    // Connection type selection
    ImGui::RadioButton("RFCOMM", _connType, RFCOMM);
    ImGui::RadioButton("L2CAP Sequential Packet", _connType, L2CAPSeqPacket);
    ImGui::RadioButton("L2CAP Stream", _connType, L2CAPStream);
    ImGui::RadioButton("L2CAP Datagram", _connType, L2CAPDgram);

    ImGui::Spacing();
    return ImGui::Button("Connect");
}

bool SDPWindow::_drawSDPList(const BTUtils::SDPResultList& list) {
    ImGui::BeginChildSpacing("sdpList", _isNew ? 0 : 1, true);
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
                _serviceName = serviceName;
                _sdpPort = port;
                ret = true;
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    ImGui::EndChild();
    return ret;
}

void SDPWindow::_drawConnectionOptions(uint16_t port, std::string_view extraInfo) {
    if (_drawBTConnOptions())
        _isNew = _list.add<ConnWindow>(Sockets::DeviceData{ _connType, _target.name, _target.address, port }, extraInfo);
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

            if (_drawSDPList(list)) ImGui::OpenPopup("options");
            if (ImGui::BeginPopup("options")) {
                _drawConnectionOptions(_sdpPort, _serviceName);
                ImGui::EndPopup();
            }
        }
    }, _sdpInquiry);
}

void SDPWindow::_drawSDPOptions() {
    if (!ImGui::BeginTabItem("Connect with SDP")) return;

    ImGui::BeginDisabled(std::holds_alternative<AsyncSDPInquiry>(_sdpInquiry));

    // UUID selection combobox
    ImGui::SetNextItemWidth(150);
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
            _sdpInquiry = std::async(std::launch::async, BTUtils::sdpLookup, _target.address, _uuids.at(_selectedUUID),
                                     _flushCache);
        } catch (const std::system_error& error) {
            _sdpInquiry = error;
        }
    }

    ImGui::EndDisabled();
    _checkInquiryStatus();
    ImGui::EndTabItem();
}

void SDPWindow::_drawManualOptions() {
    if (!ImGui::BeginTabItem("Connect Manually")) return;

    ImGui::SetNextItemWidth(100);
    ImGui::InputScalar("Port", _port, 1, 10);

    _drawConnectionOptions(_port, std::format("Port {}", _port));
    ImGui::EndTabItem();
}

void SDPWindow::_updateContents() {
    if (ImGui::BeginTabBar("ConnectionOptions")) {
        _drawSDPOptions();
        _drawManualOptions();
        ImGui::EndTabBar();
    }

    // If the connection exists, show a message
    if (!_isNew) ImGui::Text("This connection is already open.");
}
