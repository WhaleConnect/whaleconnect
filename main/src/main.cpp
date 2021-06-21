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
#include "formatcompat.hpp"

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
	if (!initApp()) return EXIT_FAILURE;

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
	static Sockets::BTSearchResult searchResult;

	if (ImGui::Button("Search for Devices")) {
		// If the search action was initiated, make sure that the background searching thread isn't already running to
		// prevent spawning multiple threads
		if (!searchRunning) {
			std::thread([&] {
				isNew = true; // Hide the "connection is open" message
				searchRunning = true; // Signal that the search thread is now running
				searchResult = Sockets::searchBT(); // Perform search
				searchRunning = false; // Done searching
			}).detach();
		}
	}

	if (searchRunning) {
		// While the search is running in the background thread, display a text spinner
		// (from https://github.com/ocornut/imgui/issues/1901#issuecomment-400563921)
		ImGui::Text("Searching... %c", "|/-\\"[static_cast<int>(ImGui::GetTime() / 0.05f) & 3]);
	} else {
		Sockets::SocketError err = searchResult.err;
		if (err.code == 0) {
			// "Display Advanced Info" checkbox to show information about the device
			static bool displayAdvanced = false;
			ImGui::Checkbox("Display Advanced Info", &displayAdvanced);
			ImGui::HelpMarker("Show technical details about a device in its entry and on hover.");

			// Search succeeded, display all devices found
			ImGui::BeginChild("DeviceList");
			for (const auto& i : searchResult.foundDevices) {
				// Check if a connection can be made, look for a valid channel (if it's not 0 it was found successfully)
				bool canConnect = i.port != 0;

				// Display the button
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 5, 5 }); // Larger padding
				std::string buttonText = i.name;

				// Format the address and channel into the device entry if advanced info is enabled
				if (displayAdvanced) buttonText += std::format(" ({} channel {})", i.address, i.port);

				if (!canConnect) {
					// Get button color
					ImVec4 disabled = ImGui::GetStyle().Colors[ImGuiCol_Button];

					// Change the hover color and click color to the original idle color. This makes the button
					// appear unresponsive, making it look disabled.
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, disabled);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, disabled);

					// Prepend a warning symbol (U+26A0) to let the user know there's a problem with this device
					buttonText = "\u26A0 " + buttonText;
				}

				// Display the button
				if (ImGui::Button(buttonText.c_str(), { -FLT_MIN, 0 })) if (canConnect) isNew = openNewConnection(i);

				if (!canConnect) ImGui::PopStyleColor(2); // Remove the disabled button colors
				ImGui::PopStyleVar(); // Remove the larger inner padding

				// Display a tooltip
				if (!canConnect && ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);

					ImGui::Text("Can't connect to this device.");

					// Based on if advanced info is enabled, show a description of why a connection can't be made
					if (displayAdvanced) ImGui::Text("The channel could not be obtained. The device may not be "
						"advertising an SDP session with the protocol selected.");
					else ImGui::Text("Enable \"Display Advanced Info\" to see more.");

					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}
			}
			ImGui::EndChild();
		} else {
			// Error occurred
			ImGui::Text("An error occurred: %d: %s", err.code, err.desc.c_str());
		}
	}

	// If the connection exists, show a message
	if (!isNew) {
		ImGui::Separator();
		ImGui::Text("A connection to this device is already open.");
	}

	ImGui::EndTabItem();
}
