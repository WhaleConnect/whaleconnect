// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <string>

#include <imgui.h>

module gui.newconnip;
import gui.imguiext;
import gui.newconn;
import net.enums;
import windows.windowlist;

void drawIPConnectionTab(WindowList& connections) {
    if (!ImGui::BeginTabItem("Internet Protocol")) return;
    ImGui::BeginChild("Output");

    using enum ConnectionType;

    static std::string addr;          // Server address
    static uint16_t port = 0;         // Server port
    static ConnectionType type = TCP; // Type of connection to create

    // Widgets
    using namespace ImGui::Literals;

    static const char* portLabel = "Port";
    static const char* addressLabel = "Address";
    static const float portWidth = 7_fh;        // Port input width (hardcoded)
    static const float minAddressWidth = 10_fh; // Address input min width

    // The horizontal space available in the window
    float spaceAvailable = ImGui::GetContentRegionAvail().x              // Child window width without scrollbars
                         - ImGui::CalcTextWidthWithSpacing(addressLabel) // Address input label width
                         - ImGui::GetStyle().ItemSpacing.x               // Space between address and port inputs
                         - ImGui::CalcTextWidthWithSpacing(portLabel)    // Port input label width
                         - portWidth;                                    // Port input width

    // Server address, set the textbox width to the space not taken up by everything else
    // Use std::max to set a minimum size for the texbox; it will not resize past a certain min bound.
    ImGui::SetNextItemWidth(std::max(spaceAvailable, minAddressWidth));
    ImGui::InputText(addressLabel, addr);

    // Server port, keep it on the same line as the textbox if there's enough space
    if (spaceAvailable > minAddressWidth) ImGui::SameLine();
    ImGui::SetNextItemWidth(portWidth);
    ImGui::InputScalar(portLabel, port, 1, 10);

    // Connection type selection
    ImGui::RadioButton("TCP", type, TCP);
    ImGui::RadioButton("UDP", type, UDP);

    // Connect button
    ImGui::Spacing();
    ImGui::BeginDisabled(addr.empty());

    if (ImGui::Button("Connect")) addConnWindow<SocketTag::IP>(connections, { type, "", addr, port }, "");

    ImGui::EndDisabled();

    ImGui::EndChild();
    ImGui::EndTabItem();
}
