// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <format>
#include <future>
#include <variant>

#include <imgui.h>

module components.sdpwindow;
import gui.imguiext;
import gui.newconn;
import net.btutils;
import utils.overload;

void printUUID(BTUtils::UUID128 uuid) {
    const uint8_t* u = uuid.data();
    ImGui::BulletText("%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X", u[0], u[1], u[2], u[3],
                      u[4], u[5], u[6], u[7], u[8], u[9], u[10], u[11], u[12], u[13], u[14], u[15]);
}

// Prints the details of a SDP result.
void drawServiceDetails(const BTUtils::SDPResult& result) {
    // Print the description (if there is one)
    ImGui::Text("Description: %s", result.desc.empty() ? "(none)" : result.desc.c_str());

    // Print UUIDs
    if (!result.protoUUIDs.empty()) ImGui::Text("Protocol UUIDs:");
    for (auto i : result.protoUUIDs) ImGui::BulletText("0x%04X", i);

    // Print service class UUIDs
    if (!result.serviceUUIDs.empty()) ImGui::Text("Service class UUIDs:");
    for (const auto& i : result.serviceUUIDs) printUUID(i);

    // Print profile descriptors
    if (!result.profileDescs.empty()) ImGui::Text("Profile descriptors:");
    for (const auto& [uuid, verMajor, verMinor] : result.profileDescs)
        ImGui::BulletText("0x%04X (version %d.%d)", uuid, verMajor, verMinor);

    // Print the port
    ImGui::Text("Port: %d", result.port);
}

bool SDPWindow::drawSDPList(const BTUtils::SDPResultList& resultList) {
    // Begin a scrollable child window to contain the list
    ImGui::BeginChild("sdpList", {}, true);
    bool ret = false;

    // ID to use in case multiple services have the same name
    int id = 0;
    for (const auto& result : resultList) {
        ImGui::PushID(id); // Push the ID, then increment it
        id++;              // TODO: Use views::enumerate() in C++23

        if (const char* name = result.name.empty() ? "Unnamed service" : result.name.c_str(); ImGui::TreeNode(name)) {
            drawServiceDetails(result);

            // Connection options
            if (ImGui::Button("Connect...")) {
                serviceName = name;
                port = result.port;
                ret = true;
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    ImGui::EndChild();
    return ret;
}

void SDPWindow::drawConnOptions(std::string_view info) {
    using enum ConnectionType;

    // Connection type selection
    ImGuiExt::radioButton("RFCOMM", connType, RFCOMM);
    ImGuiExt::radioButton("L2CAP", connType, L2CAP);

    // Connect button
    ImGui::Spacing();
    if (ImGui::Button("Connect"))
        addConnWindow<SocketTag::BT>(list, { connType, target.name, target.address, port }, info);
}

void SDPWindow::checkInquiryStatus() {
    Overload visitor{
        [](std::monostate) { ImGui::TextUnformatted("No inquiry run"); },
        [this](AsyncSDPInquiry& asyncInq) {
            using namespace std::literals;
            if (asyncInq.wait_for(0s) != std::future_status::ready) {
                // Running, display a spinner
                ImGui::TextUnformatted("Running SDP inquiry");
                ImGui::SameLine();
                ImGuiExt::spinner();
                return;
            }

            // The async operation has completed, attempt to get its results
            try {
                sdpInquiry = asyncInq.get();
            } catch (const System::SystemError& error) {
                sdpInquiry = error;
            }
        },
        [](const std::system_error&) { ImGui::TextWrapped("System error: Failed to launch thread."); },
        [](const System::SystemError& error) { ImGui::TextWrapped("Error %s", error.what()); },
        [this](const BTUtils::SDPResultList& resultList) {
            // Done, print results
            if (resultList.empty()) {
                ImGui::Text("No SDP results found for \"%s\".", target.name.c_str());
                return;
            }

            if (drawSDPList(resultList)) ImGui::OpenPopup("options");

            if (ImGui::BeginPopup("options")) {
                drawConnOptions(serviceName);
                ImGui::EndPopup();
            }
        },
    };

    std::visit(visitor, sdpInquiry);
}

void SDPWindow::drawSDPTab() {
    if (!ImGui::BeginTabItem("Connect with SDP")) return;

    // Disable the widgets if the async inquiry is running
    ImGui::BeginDisabled(std::holds_alternative<AsyncSDPInquiry>(sdpInquiry));

    // UUID selection combobox
    using namespace ImGuiExt::Literals;

    ImGui::SetNextItemWidth(10_fh);
    if (ImGui::BeginCombo("Protocol/Service UUID", selectedUUID.c_str())) {
        for (const auto& [name, _] : uuids)
            if (ImGui::Selectable(name.c_str())) selectedUUID = name;
        ImGui::EndCombo();
    }

    if constexpr (OS_WINDOWS || OS_MACOS) {
        // Flush cache option (Windows/macOS only)
        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x * 4);
        ImGui::Checkbox("Flush cache", &flushCache);
        ImGuiExt::helpMarker("Ignore previous cached advertising data on this inquiry.");
    }

    // Run button
    if (ImGui::Button("Run SDP Inquiry")) {
        try {
            // Start the inquiry
            sdpInquiry = std::async(std::launch::async, BTUtils::sdpLookup, target.address, uuids.at(selectedUUID),
                                    flushCache);
        } catch (const std::system_error& error) {
            sdpInquiry = error;
        }
    }

    ImGui::EndDisabled();
    checkInquiryStatus();
    ImGui::EndTabItem();
}

void SDPWindow::drawManualTab() {
    if (!ImGui::BeginTabItem("Connect Manually")) return;

    using namespace ImGuiExt::Literals;
    ImGui::SetNextItemWidth(7_fh);
    ImGuiExt::inputScalar("Port", port, 1, 10);

    drawConnOptions(std::format("Port {}", port));
    ImGui::EndTabItem();
}

void SDPWindow::onBeforeUpdate() {
    using namespace ImGuiExt::Literals;

    ImGui::SetNextWindowSize(30_fh * 18_fh, ImGuiCond_Appearing);
    setClosable(!std::holds_alternative<AsyncSDPInquiry>(sdpInquiry));
}

void SDPWindow::onUpdate() {
    if (ImGui::BeginTabBar("ConnectionOptions")) {
        drawSDPTab();
        drawManualTab();
        ImGui::EndTabBar();
    }
}
