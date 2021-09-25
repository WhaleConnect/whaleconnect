// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <imgui/imgui.h>

#include "app/settings.hpp"
#include "app/mainhandle.hpp"
#include "gui/console.hpp"
#include "gui/connwindowlist.hpp"
#include "net/btutils.hpp"
#include "util/imguiext.hpp"
#include "util/formatcompat.hpp"
#include "util/asyncfunction.hpp"

// List of open windows
static ConnWindowList connections(Sockets::createClientSocket);

// Console containing error/info items
static Console errorOutput;

/// <summary>
/// Render the "New Connection" window for Internet Protocol connections (TCP/UDP).
/// </summary>
static void drawIPConnectionTab();

/// <summary>
/// Render the "New Connection" window for Bluetooth connections.
/// </summary>
static void drawBTConnectionTab();

int MAIN_FUNC(MAIN_ARGS) {
    if (!MainHandler::initApp()) return EXIT_FAILURE; // Create a main application window
    int init = Sockets::init(); // Initialize sockets
    if (init != NO_ERROR) errorOutput.addError("WSAStartup failed - " + Sockets::formatErr(init));

    // Main loop
    while (MainHandler::isActive()) {
        MainHandler::handleNewFrame();
        BTUtils::glibMainContextIteration(); // (This only does stuff on Linux)

        static bool firstLoop = true; // If this is the first iteration of the main loop

        const char* newConnectionTitle = "New Connection";
        const char* errorListTitle = "Error List";

        if (firstLoop) {
            // Set up the initial docking positions
            ImGuiID id = ImGui::GetID("MainWindowGroup");
            ImVec2 pos = ImGui::GetMainViewport()->WorkPos;
            ImVec2 size{ 600, 250 };

            ImGui::DockBuilderRemoveNode(id); // Clear any previous layout
            ImGui::DockBuilderAddNode(id);
            ImGui::DockBuilderSetNodeSize(id, size);
            ImGui::DockBuilderSetNodePos(id, { pos.x + 50, pos.y + 50 });

            // Split the dock node
            ImGuiID dockLeft = ImGui::DockBuilderSplitNode(id, ImGuiDir_Left, 1, nullptr, &id);

            // Add the windows to the node
            ImGui::DockBuilderDockWindow(newConnectionTitle, dockLeft);
            ImGui::DockBuilderDockWindow(errorListTitle, dockLeft);
            ImGui::DockBuilderFinish(id);
        }

        // New connection window
        if (ImGui::Begin(newConnectionTitle)) {
            if (ImGui::BeginTabBar("ConnectionTypes")) {
                drawIPConnectionTab();
                drawBTConnectionTab();
                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        // Error list
        if (ImGui::Begin(errorListTitle)) errorOutput.update();
        ImGui::End();

        // Set the initial focus to the new connection window
        if (firstLoop) ImGui::SetWindowFocus(newConnectionTitle);

        // Poll sockets if startup was successful
        if (init == NO_ERROR) {
            // Update the ConnWindowList
            int pollRet = connections.update();

            // Check for errors
            if (pollRet == SOCKET_ERROR) errorOutput.addError("Client polling failed - " + Sockets::formatLastErr());
        }

        if (firstLoop) firstLoop = false;
        MainHandler::renderWindow();
    }

    Sockets::cleanup();
    MainHandler::cleanupApp();
    return EXIT_SUCCESS;
}

static void drawIPConnectionTab() {
    if (!ImGui::BeginTabItem("Internet Protocol")) return;
    using Sockets::ConnectionType;

    static std::string addr = ""; // Server address
    static uint16_t port = 0; // Server port
    static ConnectionType type = ConnectionType::TCP; // Type of connection to create
    static bool isNew = true; // If the attempted connection is unique

    ImGui::BeginChild("Output", { 0, (isNew) ? 0 : -ImGui::GetFrameHeightWithSpacing() });

    // Widget labels
    static const char* portLabel = "Port";
    static const char* addressLabel = "Address";

    static float portWidth = 100.0f; // The width of the port input (hardcoded)
    static float minAddressWidth = 120.0f; // The minimum width of the address textbox

    // The horizontal space available in the window
    float spaceAvail
        = ImGui::GetWindowWidth()                       // The entire width of the child window
        - ImGui::GetCurrentWindow()->ScrollbarSizes.x   // Horizontal space taken up by the vertical scrollbar
        - ImGui::CalcTextWidthWithSpacing(addressLabel) // Width of address input label
        - ImGui::GetStyle().ItemSpacing.x               // Space between the address and port inputs
        - ImGui::CalcTextWidthWithSpacing(portLabel)    // Width of the port input label
        - portWidth;                                    // Width of the port input

    // Server address, set the textbox width to the space not taken up by everything else
    // Use `ImMax()` to set a minimum size for the texbox; it will not resize past a certain min bound.
    ImGui::SetNextItemWidth(ImMax(spaceAvail, minAddressWidth));
    ImGui::InputText(addressLabel, addr);

    // Server port, keep it on the same line as the textbox if there's enough space
    if (spaceAvail > minAddressWidth) ImGui::SameLine();
    ImGui::SetNextItemWidth(portWidth);
    ImGui::InputScalar(portLabel, port, 1, 10);

    // Connection type selection
    if (ImGui::RadioButton("TCP", type == ConnectionType::TCP)) type = ConnectionType::TCP;
    if (ImGui::RadioButton("UDP", type == ConnectionType::UDP)) type = ConnectionType::UDP;

    // Connect button
    ImGui::Spacing();
    ImGui::BeginDisabled(addr.empty());
    if (ImGui::Button("Connect")) isNew = connections.add({ type, "", addr, port, 0 });
    ImGui::EndDisabled();
    ImGui::EndChild();

    // If the connection exists, show a message
    if (!isNew) {
        ImGui::Separator();
        ImGui::TextUnformatted("This connection is already open.");
    }

    ImGui::EndTabItem();
}

static void drawBTConnectionTab() {
    if (!ImGui::BeginTabItem("Bluetooth")) return;
    using Sockets::DeviceData;

    static bool isNew = true; // If the attempted connection is unique
    static bool shouldOpenNew = true; // If a new connection should be opened
    static DeviceData selected{}; // The device selected in the menu
    static AsyncFunction<uint8_t> sdpInq; // Asynchronous SDP inquiry to get channel

    bool btInitDone = BTUtils::initialized(); // If Bluetooth initialization completed
    bool sdpRunning = false; // If the SDP inquiry is still running

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::BeginChild("Output", { 0, (isNew) ? 0 : -ImGui::GetFrameHeightWithSpacing() }, false, windowFlags);
    ImGui::TextUnformatted("Paired Devices");
    ImGui::Spacing();

#ifndef _WIN32
    // Check if the application's DBus is connected to bluetoothd on Linux
    if (!btInitDone) {
        ImGui::LoadingSpinner(BTUtils::glibDisconnectedMessage);
        ImGui::Spacing();
    }
#endif

    // Check status of SDP inquiry
    if (sdpInq.error()) {
        // Error occurred
        ImGui::TextUnformatted("System error - Failed to launch thread.");
    } else if (sdpInq.checkDone()) {
        // Done successfully
        sdpRunning = false;

        static bool channelValid = false;
        if (shouldOpenNew) {
            selected.port = sdpInq.value(); // Set the port
            channelValid = (selected.port != 0);
            if (channelValid) isNew = connections.add(selected);
            shouldOpenNew = false;
        }

        if (!channelValid) ImGui::Text("Failed to get SDP channel for \"%s\".", selected.name.c_str());
    } else if (sdpInq.firstRun()) {
        // Running, display a spinner
        ImGui::LoadingSpinner("Running SDP inquiry");
        sdpRunning = true;
    }

    ImGui::Spacing();
    ImGui::BeginDisabled(!btInitDone || sdpRunning);

    // Get the paired devices when this tab is firstLoop clicked or if the "Refresh" button is clicked
    static bool firstRun = false; // If device enumeration has completed at least once
    static int pairedRet = NO_ERROR; // The return value of BTUtils::getPaired()
    static std::vector<DeviceData> pairedDevices; // Vector of paired devices
    static std::string errStr; // Error string that occurred during device enumeration (cached)
    if ((ImGui::Button("Refresh") || !firstRun) && btInitDone) {
        firstRun = true;
        pairedRet = BTUtils::getPaired(pairedDevices);

        // Set the error string if the operation failed
        // Not only does having a cache prevent extra operations from running each frame, but it minimizes the chances
        // of getting a wrong error value since every socket operation uses getLastErr() as a means of error checking.
        // If this string were to be reevaluated every frame, it might eventually pick up a wrong or misleading
        // getLastErr() value caused by some other failed action.
        if (pairedRet == SOCKET_ERROR) errStr = "[ERROR] " + Sockets::formatLastErr();
    }

    static bool showAddrs = false; // If device addresses are shown
    ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x * 4);
    ImGui::Checkbox("Show Addresses", &showAddrs);
    ImGui::Spacing();

    ImGui::BeginChild("DeviceList");
    if (pairedRet == NO_ERROR) {
        // Search succeeded, display all devices found
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 5, 5 }); // Larger padding
        for (const auto& i : pairedDevices) {
            std::string buttonText = i.name; // Button text
            if (showAddrs) buttonText += std::format(" ({})", i.address); // Format the address into the entry

            const char* addrCStr = i.address.c_str();
            ImGui::PushID(addrCStr); // Set the address as the id for when devices have the same name
            if (ImGui::Button(buttonText.c_str(), { -FLT_MIN, 0 })) {
                selected = i;
                sdpInq.run(BTUtils::getSDPChannel, addrCStr);
                shouldOpenNew = true;
            }
            ImGui::PopID();
        }
        ImGui::PopStyleVar();

        // If there are no paired devices, display a message
        // (The above loop won't run if the vector is empty, so it's not in an if-condition.)
        if (pairedDevices.empty() && btInitDone) ImGui::TextUnformatted("No paired devices.");
    } else {
        // Error occurred
        ImGui::TextUnformatted(errStr);
    }

    ImGui::EndChild();
    ImGui::EndDisabled();
    ImGui::EndChild();

    // If the connection exists, show a message
    if (!isNew) {
        ImGui::Separator();
        ImGui::TextUnformatted("A connection to this device is already open.");
    }

    ImGui::EndTabItem();
}
