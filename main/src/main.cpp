// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector> // std::vector
#include <memory> // std::unique_ptr

#ifdef _WIN32
#include <winsock2.h>
#else
#include <csignal> // Signal handling for Unix
#endif

#include <imgui/imgui.h>

#include "uicomp.hpp"
#include "util.hpp"
#include "imguiext.hpp"
#include "searchbt.hpp"
#include "mainhandle.hpp"

// Vector of open windows
std::vector<std::unique_ptr<ClientWindow>> connections;

bool openNewConnection(const DeviceData& data);
void drawIPConnectionTab();
void drawBTConnectionTab();

#if defined(_WIN32) && defined(_MSC_VER)
int CALLBACK WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
#else
int main(int, char**) {
#endif
	// Create a main application window
	if (!initApp(1280, 720, "Network Socket Terminal")) return EXIT_FAILURE;

#ifdef _WIN32
	// Start Winsock on Windows
	WSADATA wsaData;
	int startupRet = WSAStartup(MAKEWORD(2, 2), &wsaData); // MAKEWORD(2, 2) for Winsock 2.2
#else
	// SIGPIPE throws on Unix when a socket disconnects, ignore it
	std::signal(SIGPIPE, SIG_IGN);
#endif

	// Main loop
	while (isActive()) {
		handleNewFrame();

#ifdef _WIN32
		// Show error overlay if socket startup failed (Windows only)
		if (startupRet != NO_ERROR) ImGui::Overlay({ 10, 10 }, "Winsock startup failed with error %d", startupRet);
#endif

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
			if (connections[i]->open) connections[i]->update(); // Window is open, update it
			else connections.erase(connections.begin() + i); // Window is closed, remove it from vector
		}

		renderWindow();
	}

#ifdef _WIN32
	// Cleanup Winsock
	WSACleanup();
#endif

	cleanupApp();

	return EXIT_SUCCESS;
}

/// <summary>
/// Create a new connection window with given address, port, and type.
/// </summary>
/// <param name="address">The address of the remote host</param>
/// <param name="port">The port number of the remote host</param>
/// <param name="type">The type of connection to create</param>
/// <returns>If the connection window was created (true if created, false if it already exists)</returns>
bool openNewConnection(const DeviceData& data) {
	// Get the hypothetical ClientWindow title of the DeviceData struct if it were to exist
	std::string dataTitle = UIHelpers::makeClientWindowTitle(data);

	// Iterate through all open windows, check if the title matches
	for (const auto& i : connections) if (i->title == dataTitle) return false;

	// If this point is reached it means that the title is unique, it is okay to create a new window
	connections.push_back(std::make_unique<ClientWindow>(data));
	return true;
}

/// <summary>
/// Render the "New Connection" window for Internet Protocol connections (TCP/UDP).
/// </summary>
void drawIPConnectionTab() {
	if (!ImGui::BeginTabItem("Internet Protocol")) return;

	// Static local variables remove the need for global variables
	static std::string addr = ""; // Server address
	static uint16_t port = 0; // Server port
	static bool isTCP = true; // Type of connection to create (default is TCP)
	static bool isNewConnection = true; // If the specified connection is unique

	// Server address
	ImGui::SetNextItemWidth(340);
	ImGui::InputText("Address", &addr);

	// Server port
	ImGui::SameLine();
	ImGui::UnsignedInputScalar("Port", port);

	// Connection type selection
	if (ImGui::RadioButton("TCP", isTCP)) isTCP = true;
	if (ImGui::RadioButton("UDP", !isTCP)) isTCP = false;

	// Connect button, but make sure the address/port are not empty to continue.
	if (ImGui::Button("Connect") && !addr.empty()) isNewConnection = openNewConnection({ !isTCP, "", addr, port, 0 });

	// If the connection exists, show a message
	if (!isNewConnection) {
		ImGui::Separator();
		ImGui::Text("A connection with this address is already open.");
	}

	ImGui::EndTabItem();
}

/// <summary>
/// Render the "New Connection" window for Bluetooth connections. Includes a "Search for Devices" button.
/// </summary>
void drawBTConnectionTab() {
	if (!ImGui::BeginTabItem("Bluetooth RFCOMM")) return;

	static std::atomic<bool> isNew = true;
	static std::atomic<bool> searchRunning = false;
	static std::pair<int, std::vector<DeviceData>> foundDevices{ 0, {} };

	if (ImGui::Button("Search for Devices")) {
		// If the search action was initiated, make sure that the background searching thread isn't already running to
		// prevent spawning multiple threads
		if (!searchRunning) {
			std::thread([&] {
				isNew = true; // Hide the "connection is open" message
				searchRunning = true; // Signal that the search thread is now running
				foundDevices = Sockets::searchBluetoothDevices(); // Perform search
				searchRunning = false; // Done searching
			}).detach();
		}
	}

	if (searchRunning) {
		// While the search is running in the background thread, display a text spinner
		// (from https://github.com/ocornut/imgui/issues/1901#issuecomment-400563921)
		ImGui::Text("Searching... %c", "|/-\\"[static_cast<int>(ImGui::GetTime() / 0.05f) & 3]);
	} else {
		int returnCode = foundDevices.first;
		if (returnCode == 0) {
			// "Display Advanced Info" checkbox to show information about the device
			static bool displayAdvanced = false;
			ImGui::Checkbox("Display Advanced Info", &displayAdvanced);
			ImGui::HelpMarker("Show technical details about a device on hover.");

			// Search succeeded, display all devices found
			for (const auto& i : foundDevices.second) {
				// Check if a connection can be made, look for a valid channel (if it's not 0 it was found successfully)
				bool canConnect = i.port != 0;

				// Display the button
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 5, 5 }); // Larger padding
				if (ImGui::Button(i.name.c_str(), { -FLT_MIN, 0 })) if (canConnect) isNew = openNewConnection(i);
				ImGui::PopStyleVar();

				// Display a tooltip
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);

					if (canConnect) {
						// A connection can be made, show a simple description
						ImGui::Text("Connect to this device");
					} else {
						// Can't connect
						ImGui::Text("Can't connect to this device.");

						// Based on if advanced info is enabled, show a description of why a connection can't be made
						if (displayAdvanced) ImGui::Text("The channel could not be obtained. The device may not be "
							"advertising an SDP session with the protocol selected.");
						else ImGui::Text("Enable \"Display Advanced Info\" to see more.");
					}

					// Show address and channel
					if (displayAdvanced) ImGui::Text("Address: %s, Channel: %d", i.address.c_str(), i.port);

					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}
			}
		} else {
			// Error occurred
			ImGui::Text("An error occurred: %d", returnCode);
		}
	}

	// If the connection exists, show a message
	if (!isNew) {
		ImGui::Separator();
		ImGui::Text("A connection to this device is already open.");
	}

	ImGui::EndTabItem();
}
