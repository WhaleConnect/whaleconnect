// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector> // std::vector
#include <memory> // std::unique_ptr

#include <imgui/imgui.h>

#include "error.hpp"
#include "uicomp.hpp"
#include "util.hpp"
#include "imguiext.hpp"
#include "searchbt.hpp"
#include "mainhandle.hpp"
#include "formatcompat.hpp"

// Vector of open windows
std::vector<std::unique_ptr<ConnWindow>> connections;

bool openNewConnection(const DeviceData& data);
void drawIPConnectionTab();
void drawBTConnectionTab();

#if defined(_WIN32) && defined(_MSC_VER)
int CALLBACK WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
#else
int main(int, char**) {
#endif
    if (!initApp()) return EXIT_FAILURE; // Create a main application window
    int startupRet = Sockets::init(); // Initialize sockets

    // Main loop
    while (isActive()) {
        handleNewFrame();

        // Show error overlay if socket startup failed
        if (startupRet != NO_ERROR) ImGui::Overlay({ 10, 10 }, "Winsock startup failed with error %d", startupRet);

        // "New Connection" window
        ImGui::SetNextWindowSizeConstraints({ 530, 150 }, { FLT_MAX, FLT_MAX });
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
            if (connections[i]->open) connections[i]->update(); // Window is open, update it
            else connections.erase(connections.begin() + i); // Window is closed, remove it from vector
        }

        renderWindow();
    }

    Sockets::cleanup();
    cleanupApp();
    return EXIT_SUCCESS;
}

/// <summary>
/// Create a new connection window with given address, port, and type.
/// </summary>
/// <param name="data">The remote host to connect to</param>
/// <returns>If the connection window was created (true if created, false if it already exists)</returns>
bool openNewConnection(const DeviceData& data) {
    // Format the DeviceData into a usable id
    std::string id = UIHelpers::makeClientString(data, false);

    // Iterate through all open windows, check if the id matches
    for (const auto& i : connections) if (i->id == id) return false;

    // If this point is reached it means that the window is unique, it is okay to create it

    // The connector function for the ConnWindow
    auto connFunc = [](const std::atomic<bool>& sig, const DeviceData& _data) -> ConnectResult {
        // Create the client socket with the given DeviceData and stop signal
        SOCKET sockfd = Sockets::createClientSocket(_data, sig);

        // Small delay to prevent the function from finishing too fast
        std::this_thread::sleep_for(std::chrono::microseconds(100));

        // Return result
        return { sockfd, Sockets::getLastErr() };
    };

    // Append the ConnWindow to the vector
    std::string title = UIHelpers::makeClientString(data, true);
    connections.push_back(std::make_unique<ConnWindow>(title, id, connFunc, data));
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
    static bool isNew = true; // If the specified connection is unique

    // Server address
    ImGui::SetNextItemWidth(340);
    ImGui::InputText("Address", &addr);

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
        ImGui::Text("This connection is already open.");
    }

    ImGui::EndTabItem();
}

/// <summary>
/// Render the "New Connection" window for Bluetooth connections. Includes a "Search for Devices" button.
/// </summary>
void drawBTConnectionTab() {
    if (!ImGui::BeginTabItemNoSpacing("Bluetooth RFCOMM")) return;

    static bool firstRun = false; // If a search has been performed at least once
    static bool done = false; // If the search has completed and returned a result
    static bool isNew = true; // If the specified connection is unique
    static bool error = false; // If the async function failed to start
    static std::future<Sockets::BTSearchResult> fut; // The future  to run the search in parallel
    static Sockets::BTSearchResult result; // The result of the search

    ImGui::PushDisabled(!done && firstRun);
    if (ImGui::Button("Search for Devices")) {
        isNew = true; // Hide the "connection is open" message
        firstRun = true; // The search has been run
        done = false; // Search is in progress

        try {
            fut = std::async(std::launch::async, Sockets::searchBT);
        } catch (const std::system_error&) {
            error = done = true;
        }
    }
    ImGui::PopDisabled();

    // Check the state of the search
    if (fut.valid() && (fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready)) {
        done = true;
        result = fut.get();
    }

    ImVec2 reservedSpace = { 0, (isNew) ? 0 : -ImGui::GetFrameHeightWithSpacing() };
    ImGui::BeginChild("Output", reservedSpace, false, ImGuiWindowFlags_HorizontalScrollbar);
    if (done && !error) {
        int err = result.err;
        if (err == 0) {
            // "Display Advanced Info" checkbox to show information about the device
            static bool displayAdvanced = false;
            ImGui::Checkbox("Display Advanced Info", &displayAdvanced);
            ImGui::HelpMarker("Show technical details about a device in its entry");

            // Search succeeded, display all devices found
            for (const auto& i : result.foundDevices) {
                // Display the button
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 5, 5 }); // Larger padding
                std::string buttonText = i.name;
                const char* addr = i.address.c_str();

                // Format the address and channel into the device entry if advanced info is enabled
                if (displayAdvanced) buttonText += std::format(" ({} channel {})", addr, i.port);

                ImGui::PushDisabled(i.port == 0); // 0 is an invalid channel meaning it couldn't be found
                ImGui::PushID(addr); // Set the address (always unique) as the id in case devices have the same name
                if (ImGui::Button(buttonText.c_str(), { -FLT_MIN, 0 })) isNew = openNewConnection(i);
                ImGui::PopID();
                ImGui::PopDisabled();
                ImGui::PopStyleVar();
            }
        } else {
            // Error occurred
            Sockets::NamedError ne = Sockets::getErr(err);
            ImGui::Text("[ERROR] %s (%d): %s", ne.name, err, ne.desc);
        }
    } else {
        // If the async function failed to launch show an error
        if (error) ImGui::Text("[ERROR] System error - Failed to launch thread.");

        // While the search is running in the background thread, display a text spinner
        else if (firstRun) ImGui::LoadingSpinner("Searching");
    }
    ImGui::EndChild();

    // If the connection exists, show a message
    if (!isNew) {
        ImGui::Separator();
        ImGui::Text("A connection to this device is already open.");
    }

    ImGui::EndTabItem();
}
