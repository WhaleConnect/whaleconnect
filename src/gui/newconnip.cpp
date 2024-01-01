// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <string>
#include <string_view>

#include <imgui.h>

module gui.newconnip;
import components.windowlist;
import gui.imguiext;
import gui.newconn;
import net.enums;

// Gets the width of a rendered string added with the item inner spacing specified in the Dear ImGui style.
float calcTextWidthWithSpacing(std::string_view text) {
    return ImGui::GetStyle().ItemInnerSpacing.x + ImGui::CalcTextSize(text.data()).x;
}

void drawIPConnectionTab(WindowList& connections) {
    if (!ImGui::BeginTabItem("Internet Protocol")) return;
    ImGui::BeginChild("Output");

    using enum ConnectionType;

    static std::string addr; // Server address
    static uint16_t port = 0; // Server port
    static ConnectionType type = TCP; // Type of connection to create
    static bool useTLS = false; // If TLS is used for secure connections

    // Widgets
    using namespace ImGuiExt::Literals;

    static const char* portLabel = "Port";
    static const char* addressLabel = "Address";
    static const float portWidth = 7_fh; // Port input width (hardcoded)
    static const float minAddressWidth = 10_fh; // Address input min width

    // The horizontal space available in the window
    float emptySpace = ImGui::GetContentRegionAvail().x // Child window width without scrollbars
        - calcTextWidthWithSpacing(addressLabel) // Address input label width
        - ImGui::GetStyle().ItemSpacing.x // Space between address and port inputs
        - calcTextWidthWithSpacing(portLabel) // Port input label width
        - portWidth; // Port input width

    // Server address, set the textbox width to the space not taken up by everything else
    // Use std::max to set a minimum size for the texbox; it will not resize past a certain min bound.
    ImGui::SetNextItemWidth(std::max(emptySpace, minAddressWidth));
    ImGuiExt::inputText(addressLabel, addr);

    // Server port, keep it on the same line as the textbox if there's enough space
    if (emptySpace > minAddressWidth) ImGui::SameLine();
    ImGui::SetNextItemWidth(portWidth);
    ImGuiExt::inputScalar(portLabel, port, 1, 10);

    // Connection type selection
    ImGuiExt::radioButton("TCP", type, TCP);
    ImGuiExt::radioButton("UDP", type, UDP);

    // Connect button
    ImGui::Spacing();
    ImGui::BeginDisabled(addr.empty());

    if (ImGui::Button("Connect")) addConnWindow(connections, useTLS, { type, "", addr, port }, "");

    ImGui::EndDisabled();

    // Option to use TLS (TCP only)
    if (type == TCP) {
        ImGui::SameLine();
        ImGui::Checkbox("Use TLS", &useTLS);
    }

    ImGui::EndChild();
    ImGui::EndTabItem();
}
