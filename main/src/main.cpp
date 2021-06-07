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
#include "sockets.hpp"
#include "searchbt.hpp"
#include "mainhandle.hpp"

// Vector of open windows
std::vector<std::unique_ptr<ClientWindow>> connections;

bool createNewConnection(const DeviceData& data);
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
		if (startupRet != NO_ERROR) {
			// Window flags to make sure the window is fixed and immobile
			int flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav
				| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

			// Get main viewport
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImVec2 workPos = viewport->WorkPos;

			// Window configuration
			int padding = 10;
			ImGui::SetNextWindowBgAlpha(0.5f);
			ImGui::SetNextWindowPos({ workPos.x + padding, workPos.y + padding }, ImGuiCond_Always);
			ImGui::SetNextWindowViewport(viewport->ID);

			if (ImGui::Begin("Error", nullptr, flags)) ImGui::Text("Winsock startup failed with error %d", startupRet);
			ImGui::End();
		}
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
			else connections.erase(connections.begin() + i); // Window is closed, it from vector
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
bool createNewConnection(const DeviceData& data) {
	bool isNew = true;

	// Iterate through all open windows, check if the address is in one of the titles
	for (const auto& i : connections) {
		// Bluetooth connections use the device's name (e.g. "MyESP32") as a unique identifier,
		// IP connections (TCP/UDP) use the device's IP address (e.g. 192.168.0.164).
		const std::string& identifier = (data.type == Bluetooth) ? data.name : data.address;

		// Check if another window already has this identifier
		if (i->title.find(identifier) != std::string::npos) {
			// If this condition is true it means the connection is already open
			isNew = false;
			break; // Save some time, don't need to check further
		}
	}

	// Create new object only if the connection does not exist
	if (isNew) connections.push_back(std::make_unique<ClientWindow>(data));

	return isNew;
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
	if (ImGui::Button("Connect") && !addr.empty()) isNewConnection = createNewConnection({ !isTCP, "", addr, port, 0 });

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

	static std::atomic<bool> isNewConnection = true;
	static std::atomic<bool> searchRunning = false;
	static std::pair<int, std::vector<DeviceData>> foundDevices{ 0, {} };

	if (ImGui::Button("Search for Devices")) {
		// If the search action was initiated, make sure that the background searching thread isn't already running to
		// prevent spawning multiple threads
		if (!searchRunning) {
			std::thread([&] {
				isNewConnection = true; // Hide the "connection is open" message
				searchRunning = true; // Signal that the search thread is now running
				foundDevices = Sockets::searchBluetoothDevices(); // Perform search
				searchRunning = false; // Done searching
			}).detach();
		}
	}

	if (searchRunning) {
		// While the search is running in the background thread, display a text spinner
		ImGui::Text("Searching... %c", "|/-\\"[static_cast<int>(ImGui::GetTime() / 0.05f) & 3]);
	} else {
		int returnCode = foundDevices.first;
		if (returnCode == 0) {
			// "Display Advanced Info" checkbox to show information about the device
			static bool displayAdvanced = false;
			ImGui::Checkbox("Display Advanced Info", &displayAdvanced);

			// Search succeeded, display all devices found
			int numCols = (displayAdvanced) ? 3 : 1;
			if (ImGui::BeginTable("deviceList", numCols, ImGuiTableFlags_Borders | ImGuiTableFlags_BordersInner)) {
				// Set up table headers
				ImGui::TableSetupColumn("Device");

				if (displayAdvanced) {
					ImGui::TableSetupColumn("Address");
					ImGui::TableSetupColumn("Channel");
				}

				ImGui::TableHeadersRow();
				ImGui::TableNextColumn();

				// Retrieve device list
				auto deviceList = foundDevices.second;

				// Display the names of the found devices
				int id = 0;
				for (const auto& i : deviceList) {
					ImGui::Text(i.name);
					ImGui::SameLine();

					if (i.port == 0) {
						ImGui::Text("\u26A0");
						if (ImGui::IsItemHovered()) {
							ImGui::BeginTooltip();
							ImGui::Text("This device is unreachable.\n\n%s", (displayAdvanced)
								? "The channel could not be obtained. It may not be advertising an RFCOMM SDP session."
								: "Enable \"Display Advanced Info\" to see more.");
							ImGui::EndTooltip();
						}
					} else {
						// Display a button to connect to the device
						// Each button has the same label ("Connect"), we need to assign a unique id to each one.
						// The postfix increment operator used below will increment the id, then return the value of the
						// id before it was incremented.
						ImGui::PushID(id++);

						// Display the button, attempt to create a new connection if it's clicked
						if (ImGui::SmallButton("Connect")) isNewConnection = createNewConnection(i);

						// We're done using this id:
						ImGui::PopID();
					}
				}

				ImGui::TableNextColumn();

				// Advanced information
				if (displayAdvanced) {
					// Display the addresses of the found devices
					for (const auto& i : deviceList) ImGui::Text(i.address);
					ImGui::TableNextColumn();

					// Display the channels of the found devices
					for (const auto& i : deviceList) ImGui::Text(i.port);
					ImGui::TableNextColumn();
				}

				// End table
				ImGui::EndTable();
			}
		} else {
			// Error occurred
			ImGui::Text("An error occurred: %d", returnCode);
		}
	}

	// If the connection exists, show a message
	if (!isNewConnection) {
		ImGui::Separator();
		ImGui::Text("A connection to this device is already open.");
	}

	ImGui::EndTabItem();
}
