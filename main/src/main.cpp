// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector> // std::vector
#include <memory> // std::unique_ptr

#include <imgui/imgui.h>

#include "btutils.hpp"
#include "error.hpp"
#include "uicomp.hpp"
#include "utils.hpp"
#include "imguiext.hpp"
#include "mainhandle.hpp"
#include "formatcompat.hpp"

// Vector of open windows
std::vector<std::unique_ptr<ConnWindow>> connections;

bool openNewConnection(const DeviceData& data);
void drawIPConnectionTab();
void drawBTConnectionTab();

int MAIN_FUNC(MAIN_ARGS) {
    if (!MainHandler::initApp()) return EXIT_FAILURE; // Create a main application window
    int init = Sockets::init(); // Initialize sockets

    // Main loop
    while (MainHandler::isActive()) {
        MainHandler::handleNewFrame();
        BTUtils::glibMainContextIteration(); // (This only does stuff on Linux)

        // Show error overlay if socket startup failed
        if (init != NO_ERROR) ImGui::Overlay({ 10, 10 }, ImGuiOverlayCorner_TopLeft, "Startup failed (%d).", init);

        // "New Connection" window
        ImGui::SetNextWindowSize({ 600, 250 }, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("New Connection")) {
            if (ImGui::BeginTabBar("ConnectionTypes")) {
                drawIPConnectionTab();
                drawBTConnectionTab();
                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        // Update all client windows
        for (size_t i = 0; i < connections.size(); i++) {
            if (*connections[i]) connections[i]->update(); // Window is open, update it
            else connections.erase(connections.begin() + i); // Window is closed, remove it from vector
        }

        MainHandler::renderWindow();
    }

    Sockets::cleanup();
    MainHandler::cleanupApp();
    return EXIT_SUCCESS;
}

/// <summary>
/// Create a new connection window with given address, port, and type.
/// </summary>
/// <param name="data">The remote host to connect to</param>
/// <returns>If the connection window was created (true if created, false if it already exists)</returns>
bool openNewConnection(const DeviceData& data) {
    // Format the DeviceData into a usable title
    std::string title = UIHelpers::makeClientString(data);

    // Iterate through all open windows, check if the title matches
    for (const auto& i : connections) if (*i == title) return false;

    // If this point is reached it means that the window is unique, it is okay to create it

    // The connector function for the ConnWindow
    auto connFunc = [](const std::atomic<bool>& sig, const DeviceData& _data) -> ConnectResult {
        // Create the client socket with the given DeviceData and stop signal
        SOCKET sockfd = Sockets::createClientSocket(_data, sig);
        return { sockfd, Sockets::getLastErr() };
    };

    // Append the ConnWindow to the vector
    connections.push_back(std::make_unique<ConnWindow>(title, connFunc, data));
    return true;
}

/// <summary>
/// Render the "New Connection" window for Internet Protocol connections (TCP/UDP).
/// </summary>
void drawIPConnectionTab() {
    if (!ImGui::BeginTabItemNoSpacing("Internet Protocol")) return;

    static std::string addr = ""; // Server address
    static uint16_t port = 0; // Server port
    static bool isTCP = true; // Type of connection to create (default is TCP)
    static bool isNew = true; // If the attempted connection is unique

    // Server address
    ImGui::SetNextItemWidth(340);
    ImGui::InputText("Address", addr);

    // Server port
    ImGui::SameLine();
    ImGui::UnsignedInputScalar("Port", port);

    // Connection type selection
    if (ImGui::RadioButton("TCP", isTCP)) isTCP = true;
    if (ImGui::RadioButton("UDP", !isTCP)) isTCP = false;

    // Connect button
    ImGui::PushDisabled(addr.empty());
    if (ImGui::Button("Connect")) isNew = openNewConnection({ !isTCP, "", addr, port, 0 });
    ImGui::PopDisabled();

    // If the connection exists, show a message
    if (!isNew) {
        ImGui::Separator();
        ImGui::TextUnformatted("This connection is already open.");
    }

    ImGui::EndTabItem();
}

/// <summary>
/// Render the "New Connection" window for Bluetooth connections.
/// </summary>
void drawBTConnectionTab() {
    if (!ImGui::BeginTabItemNoSpacing("Bluetooth")) return;
    ImGui::TextUnformatted("Paired Devices");

    static bool isNew = true; // If the attempted connection is unique
    static bool shouldOpenNew = true; // If a new connection should be opened
    static AsyncFunction<uint8_t, DeviceData> sdpInq; // Asynchronous SDP inquiry to get channel
    bool disableMenu = false; // If the selection menu is disabled (affected by various factors)

#ifndef _WIN32
    // Check if the application's DBus is connected to bluetoothd on Linux
    disableMenu = !BTUtils::glibConnected;
    if (disableMenu) ImGui::TextUnformatted(BTUtils::glibDisconnectedMessage);
#endif

    // Check status of SDP inquiry
    if (sdpInq.error()) {
        // Error occurred
        ImGui::TextUnformatted("System error - Failed to launch thread.");
    } else if (sdpInq.checkDone()) {
        // Done and no error, get the stored DeviceData (the one corresponding to the button the user clicked)
        static bool channelValid = false;
        DeviceData data = sdpInq.userData();
        if (shouldOpenNew) {
            data.port = sdpInq.getValue(); // Set the port
            channelValid = (data.port != 0);
            if (channelValid) isNew = openNewConnection(data);
            shouldOpenNew = false;
        }
        if (!channelValid) ImGui::Text("Failed to get SDP channel for \"%s\".", data.name.c_str());
        disableMenu = false;
    } else if (sdpInq.firstRun()) {
        // Running, display a spinner
        ImGui::LoadingSpinner("Running SDP inquiry");
        disableMenu = true;
    }

    ImGui::PushDisabled(disableMenu);

    // Get the paired devices when this tab is first clicked or if the "Refresh" button is clicked
    static bool firstRun = false; // If device enumeration has completed at least once
    static std::vector<DeviceData> pairedDevices; // Vector of paired devices
    static int err = NO_ERROR; // Error that occurred during device enumeration
    if ((ImGui::Button("Refresh") || !firstRun) && !disableMenu) {
        err = BTUtils::getPaired(pairedDevices);
        firstRun = true;
    }

    static bool showAddrs = false; // If device addresses are shown
    ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x * 4);
    ImGui::Checkbox("Show Addresses", &showAddrs);
    ImGui::Spacing();

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::BeginChild("Output", { 0, (isNew) ? 0 : -ImGui::GetFrameHeightWithSpacing() }, false, windowFlags);
    if (err == NO_ERROR) {
        // Search succeeded, display all devices found
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 5, 5 }); // Larger padding
        for (const auto& i : pairedDevices) {
            std::string buttonText = i.name; // Button text
            if (showAddrs) buttonText += std::format(" ({})", i.address); // Format the address into the entry

            const char* addrCStr = i.address.c_str();
            ImGui::PushID(addrCStr); // Set the address as the id for when devices have the same name
            if (ImGui::Button(buttonText.c_str(), { -FLT_MIN, 0 })) {
                sdpInq.userData() = i;
                sdpInq.run(BTUtils::getSDPChannel, addrCStr);
                shouldOpenNew = true;
            }
            ImGui::PopID();
        }
        ImGui::PopStyleVar();

        // If there are no paired devices, display a message
        // (The above loop won't run if the vector is empty, so it's not in an if-condition.)
        if (pairedDevices.empty() && !disableMenu) ImGui::TextUnformatted("No paired devices.");
    } else {
        // Error occurred
        Sockets::NamedError ne = Sockets::getErr(Sockets::getLastErr());
        ImGui::Text("[ERROR] %s (%d): %s", ne.name, err, ne.desc);
    }
    ImGui::EndChild();
    ImGui::PopDisabled();

    // If the connection exists, show a message
    if (!isNew) {
        ImGui::Separator();
        ImGui::TextUnformatted("A connection to this device is already open.");
    }

    ImGui::EndTabItem();
}
