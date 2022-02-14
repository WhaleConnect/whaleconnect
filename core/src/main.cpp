// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <map>
#include <iterator> // std::back_inserter()
#include <string_view>

#include "app/settings.hpp"
#include "app/mainhandle.hpp"
#include "async/async.hpp"
#include "async/asyncfunction.hpp"
#include "gui/console.hpp"
#include "gui/connwindowlist.hpp"
#include "net/btutils.hpp"
#include "sys/error.hpp"
#include "util/imguiext.hpp"
#include "util/formatcompat.hpp"

// List of open windows
static ConnWindowList connections;

// If an attempted Bluetooth connection is unique
static bool isNewBT = true;

/// <summary>
/// Render the "New Connection" window for Internet-based connections.
/// </summary>
static void drawIPConnectionTab();

/// <summary>
/// Render the "New Connection" window for Bluetooth-based connections.
/// </summary>
static void drawBTConnectionTab();

int MAIN_FUNC(MAIN_ARGS) {
    if (!MainHandler::initApp()) return EXIT_FAILURE; // Create a main application window

    // Console containing error/info items
    Console errorOutput;

    // Initialize APIs for sockets and Bluetooth
    if (BTUtils::init() != NO_ERROR) errorOutput.addError("Initialization failed - " + System::formatLastErr());

    // Main loop
    while (MainHandler::isActive()) {
        MainHandler::handleNewFrame();

        const char* newConnectionTitle = "New Connection";
        const char* errorListTitle = "Error List";

        // New connection window
        if (ImGui::Begin(newConnectionTitle) && ImGui::BeginTabBar("ConnectionTypes")) {
            drawIPConnectionTab();
            drawBTConnectionTab();
            ImGui::EndTabBar();
        }
        ImGui::End();

        // Error list
        if (ImGui::Begin(errorListTitle)) errorOutput.update();
        ImGui::End();

        if (MainHandler::isFirstLoop()) {
            // Set up the initial docking positions
            ImGuiID id = ImGui::GetID("mainWindowGroup");
            ImVec2 pos = ImGui::GetMainViewport()->WorkPos;

            // Set up nodes
            ImGui::DockBuilderRemoveNode(id); // Clear any previous layout
            ImGui::DockBuilderAddNode(id);
            ImGui::DockBuilderSetNodeSize(id, { 600, 250 });
            ImGui::DockBuilderSetNodePos(id, { pos.x + 50, pos.y + 50 }); // 50px padding in both X and Y

            // Add the windows to the node
            ImGui::DockBuilderDockWindow(newConnectionTitle, id);
            ImGui::DockBuilderDockWindow(errorListTitle, id);
            ImGui::DockBuilderFinish(id);

            // Set the initial focus to the new connection window
            ImGui::SetWindowFocus(newConnectionTitle);
        }

        connections.update();

        MainHandler::renderWindow();
    }

    BTUtils::cleanup();
    MainHandler::cleanupApp();
    return EXIT_SUCCESS;
}

static void drawIPConnectionTab() {
    if (!ImGui::BeginTabItem("Internet Protocol")) return;
    using Sockets::ConnectionType;
    using enum ConnectionType;

    static std::string addr; // Server address
    static uint16_t port = 0; // Server port
    static ConnectionType type = TCP; // Type of connection to create
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
    if (ImGui::RadioButton("TCP", type == TCP)) type = TCP;
    if (ImGui::RadioButton("UDP", type == UDP)) type = UDP;

    // Connect button
    ImGui::Spacing();
    ImGui::BeginDisabled(addr.empty());
    if (ImGui::Button("Connect")) isNew = connections.add({ type, "", addr, port });
    ImGui::EndDisabled();
    ImGui::EndChild();

    // If the connection exists, show a message
    if (!isNew) ImGui::Text("This connection is already open.");

    ImGui::EndTabItem();
}

/// <summary>
/// Draw the combobox used to select UUIDs for Bluetooth SDP lookup.
/// </summary>
/// <returns>The UUID selected in the combobox</returns>
static UUID drawUUIDCombo() {
    // Map of UUIDs, associating a UUID value with a user-given name
    static std::map<std::string, UUID> uuidList = {
        { "L2CAP", BTUtils::createUUIDFromBase(0x0100) },
        { "RFCOMM", BTUtils::createUUIDFromBase(0x0003) }
    };

    // The selected entry, start with the first one
    static std::string selected = uuidList.begin()->first;

    // Draw the combobox
    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::BeginCombo("Protocol/Service UUID", selected.c_str())) {
        for (const auto& [name, uuid] : uuidList) if (ImGui::Selectable(name.c_str())) selected = name;
        ImGui::EndCombo();
    }

    // Return the selected entry
    return uuidList[selected];
}

/// <summary>
/// Draw a menu composed of Bluetooth devices.
/// </summary>
/// <param name="devices">The list of devices to display in the menu</param>
/// <param name="showAddrs">If addresses are shown next to device names</param>
/// <param name="selected">The device selected in the menu (modified when an entry is clicked)</param>
/// <returns>If an entry was selected</returns>
bool drawPairedDevicesList(const Sockets::DeviceDataList& devices, bool showAddrs, Sockets::DeviceData& selected) {
    bool ret = false;

    // The menu is a listbox which can display 4 entries at a time
    if (ImGui::BeginListBox("##paired", { ImGui::FILL, ImGui::GetFrameHeight() * 4 })) {
        // The index of the selected item
        // The code is structured so that no buffer overruns occur if the number of devices decreases after a
        // refresh (don't use this variable to access something in the vector).
        // The initial value for the selected index is -1, so no items appear selected at first.
        // Negative values for size_t are well-defined by the standard - https://stackoverflow.com./q/28247733
        static size_t selectedIdx = static_cast<size_t>(-1);
        for (size_t i = 0; i < devices.size(); i++) {
            // The current device
            const auto& current = devices[i];
            const bool isSelected = (selectedIdx == i);

            // Get the necessary string fields, then create the device's entry in the listbox
            std::string text = current.name;
            if (showAddrs) std::format_to(std::back_inserter(text), " ({})", current.address);

            // Render the Selectable
            // Push the address (always unique) as the ID in case multiple devices have the same name.
            ImGui::PushID(current.address.c_str());
            if (ImGui::Selectable(text.c_str(), isSelected)) {
                // Something was selected
                selectedIdx = i;
                selected = current;
                ret = true;
            }
            ImGui::PopID();

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
    }

    return ret;
}

/// <summary>
/// Draw the options for connecting to a device with Bluetooth.
/// </summary>
/// <param name="target">The device to connect to</param>
/// <param name="port">The port (RFCOMM: channel, L2CAP: PSM) of the server running on the device</param>
/// <param name="extraInfo">Any extra text to display in the new window</param>
static void drawBTConnOptions(const Sockets::DeviceData& target, uint16_t port, std::string_view extraInfo) {
    using Sockets::ConnectionType;
    using enum ConnectionType;

    static ConnectionType type = RFCOMM;

    // Connection type selection
    if (ImGui::RadioButton("RFCOMM", type == RFCOMM)) type = RFCOMM;
    if (ImGui::RadioButton("L2CAP Sequential Packet", type == L2CAPSeqPacket)) type = L2CAPSeqPacket;
    if (ImGui::RadioButton("L2CAP Stream", type == L2CAPStream)) type = L2CAPStream;
    if (ImGui::RadioButton("L2CAP Datagram", type == L2CAPDgram)) type = L2CAPDgram;

    ImGui::Spacing();
    if (ImGui::Button("Connect")) isNewBT = connections.add({ type, target.name, target.address, port }, extraInfo);
}

/// <summary>
/// Begin a child window with an optional one-line spacing at the bottom.
/// </summary>
/// <param name="spacing">If spacing is reserved at the bottom of the child</param>
/// <param name="border">If a border is present around the child</param>
static void beginChildWithSpacing(bool spacing, bool border) {
    ImGui::BeginChild("output", { 0, (spacing) ? 0 : -ImGui::GetFrameHeightWithSpacing() }, border);
}

/// <summary>
/// Display the entries from an SDP lookup (with buttons to connect to each) in a tree format.
/// </summary>
/// <param name="list">The SDP lookup list to display</param>
/// <param name="selected">The DeviceData instance used as context for making a connection</param>
static void drawSDPList(const BTUtils::SDPResultList& list, const Sockets::DeviceData& selected) {
    // If no SDP results were found, display a message and exit
    if (list.empty()) {
        ImGui::Text("No SDP results found for \"%s\".", selected.name.c_str());
        return;
    }

    beginChildWithSpacing(isNewBT, true);

    // ID to use in case multiple services have the same name
    unsigned int id = 0;
    for (const auto& [protos, services, profiles, port, name, desc] : list) {
        const char* serviceName = (name.empty()) ? "Unnamed service" : name.c_str();

        ImGui::PushID(id++); // Push the ID, then increment it

        if (ImGui::TreeNode(serviceName)) {
            // Print the description (if there is one)
            ImGui::Text("Description: %s", (desc.empty()) ? "(none)" : desc.c_str());

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
            if (ImGui::Button("Connect...")) ImGui::OpenPopup("options");
            if (ImGui::BeginPopup("options")) {
                drawBTConnOptions(selected, port, serviceName);
                ImGui::EndPopup();
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    ImGui::EndChild();
}

static void drawBTConnectionTab() {
    if (!ImGui::BeginTabItem("Bluetooth")) return;

    bool btInitDone = BTUtils::initialized(); // If Bluetooth initialization completed
    static bool sdpRunning = false; // If the SDP inquiry is still running

    // Check if the application's sockets are initialized
    if (!btInitDone) {
        ImGui::TextWrapped("Socket initialization failed. See the \"Error List\" window for details.");
        ImGui::Spacing();
    }

    ImGui::BeginDisabled(!btInitDone || sdpRunning);

    // Get the paired devices when this tab is first clicked or if the "Refresh" button is clicked

    static bool devicesListed = false; // If device enumeration has completed at least once
    static System::MayFail<Sockets::DeviceDataList> pairedDevices; // Vector of paired devices
    static std::string errStr; // Error string that occurred during device enumeration (cached)

    if ((ImGui::Button("Refresh List") || !devicesListed) && btInitDone) {
        devicesListed = true;
        pairedDevices = BTUtils::getPaired();

        // Set the error string if the operation failed
        // Not only does having a cache prevent extra operations from running each frame, but it minimizes the chances
        // of getting a wrong error value since every socket operation uses getLastErr() as a means of error checking.
        // If this string were to be reevaluated every frame, it might eventually pick up a wrong or misleading
        // getLastErr() value caused by some other failed action.
        if (!pairedDevices) errStr = "[ERROR] " + System::formatLastErr();
    }

    static bool useSDP = true; // If available connections on devices are found using SDP
    static Sockets::DeviceData selected; // The device selected in the menu
    static AsyncFunction<System::MayFail<BTUtils::SDPResultList>> sdpInq; // Asynchronous SDP inquiry
    static bool deviceSelected = false; // If the user interacted with the device menu at least once

    if (pairedDevices) {
        // Enumeration succeeded, display all devices found
        if (btInitDone && pairedDevices->empty()) {
            // Bluetooth initialization is done and no devices detected
            // (BT init checked because an empty devices vector may be caused by failed init.)
            ImGui::Text("No paired devices.");
        } else {
            float sameLineSpacing = ImGui::GetStyle().ItemInnerSpacing.x * 4; // Spacing between the below widgets

            // Checkbox to display device addresses
            static bool showAddrs = false;
            ImGui::SameLine(0, sameLineSpacing);
            ImGui::Checkbox("Show Addresses", &showAddrs);
            ImGui::Spacing();

            // Checkbox to switch between SDP/manual connection modes
            // Hide the unique connection message when the mode is switched.
            if (ImGui::Checkbox("Use SDP", &useSDP)) isNewBT = true;

            static UUID selectedUUID; // UUID selection combobox
            static bool flushSDPCache = false; // If SDP cache is flushed on an inquiry (Windows only)

            if (useSDP) {
                selectedUUID = drawUUIDCombo();

#ifdef _WIN32
                // "Flush cache" option (Windows only)
                ImGui::SameLine(0, sameLineSpacing);
                ImGui::Checkbox("Flush cache", &flushSDPCache);
                ImGui::HelpMarker("Ignore previous cached advertising data on this inquiry.");
#endif
            }

            // There are devices, display them
            if (drawPairedDevicesList(*pairedDevices, showAddrs, selected)) {
                deviceSelected = true;

                if (useSDP) sdpInq.run(BTUtils::sdpLookup, selected.address, selectedUUID, flushSDPCache);
            }
        }
    } else {
        // Error occurred
        ImGui::TextWrapped("Error: %s", errStr.c_str());
    }

    ImGui::EndDisabled();

    if (useSDP) {
        // Check status of SDP inquiry
        if (sdpInq.error()) {
            // Error occurred
            ImGui::TextWrapped("System error - Failed to launch thread.");
            sdpRunning = false;
        } else if (sdpInq.checkDone()) {
            // Done, print results
            drawSDPList(*sdpInq.value(), selected);
            sdpRunning = false;
        } else if (sdpInq.firstRun()) {
            // Running, display a spinner
            ImGui::LoadingSpinner("Running SDP inquiry");
            sdpRunning = true;
        }
    } else if (deviceSelected) {
        // SDP is not used and a device is selected, show the connection options
        beginChildWithSpacing(isNewBT, false);
        static uint16_t port = 0;

        ImGui::Spacing();
        ImGui::SetNextItemWidth(100.0f);
        ImGui::InputScalar("Port", port, 1, 10);

        drawBTConnOptions(selected, port, std::format("Port {}", port));
        ImGui::EndChild();
    }

    ImGui::EndTabItem();

    // If the connection exists, show a message
    if (!isNewBT) ImGui::Text("This connection is already open.");
}
