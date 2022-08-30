// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>

#include <imgui.h>

#include "newconnip.hpp"
#include "gui/connwindow.hpp"
#include "gui/windowlist.hpp"
#include "net/sockets.hpp"
#include "util/imguiext.hpp"

void drawIPConnectionTab(WindowList& connections) {
    if (!ImGui::BeginTabItem("Internet Protocol")) return;
    using Sockets::ConnectionType;
    using enum ConnectionType;

    static std::string addr; // Server address
    static uint16_t port = 0; // Server port
    static ConnectionType type = TCP; // Type of connection to create
    static bool isNew = true; // If the attempted connection is unique

    ImGui::BeginChildSpacing("Output", !isNew);

    // Widget labels
    static const char* portLabel = "Port";
    static const char* addressLabel = "Address";

    static float portWidth = 100.0f; // The width of the port input (hardcoded)
    static float minAddressWidth = 120.0f; // The minimum width of the address textbox

    // The horizontal space available in the window
    float spaceAvailable
        = ImGui::GetContentRegionAvail().x              // The width of the child window without scrollbars
        - ImGui::CalcTextWidthWithSpacing(addressLabel) // Width of address input label
        - ImGui::GetStyle().ItemSpacing.x               // Space between the address and port inputs
        - ImGui::CalcTextWidthWithSpacing(portLabel)    // Width of the port input label
        - portWidth;                                    // Width of the port input

    // Server address, set the textbox width to the space not taken up by everything else
    // Use std::max to set a minimum size for the texbox; it will not resize past a certain min bound.
    ImGui::SetNextItemWidth(std::max(spaceAvailable, minAddressWidth));
    ImGui::InputText(addressLabel, addr);

    // Server port, keep it on the same line as the textbox if there's enough space
    if (spaceAvailable > minAddressWidth) ImGui::SameLine();
    ImGui::SetNextItemWidth(portWidth);
    ImGui::InputScalar(portLabel, port, 1, 10);

    // Connection type selection
    if (ImGui::RadioButton("TCP", type == TCP)) type = TCP;
    if (ImGui::RadioButton("UDP", type == UDP)) type = UDP;

    // Connect button
    ImGui::Spacing();
    ImGui::BeginDisabled(addr.empty());
    if (ImGui::Button("Connect")) isNew = connections.add<ConnWindow>(Sockets::DeviceData{ type, "", addr, port }, "");
    ImGui::EndDisabled();
    ImGui::EndChild();

    // If the connection exists, show a message
    if (!isNew) ImGui::Text("This connection is already open.");

    ImGui::EndTabItem();
}
