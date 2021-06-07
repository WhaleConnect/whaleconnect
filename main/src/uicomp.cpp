// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <format> // std::format() [C++20]

#include <imgui/imgui.h>

#include "uicomp.hpp"
#include "imguiext.hpp"
#include "util.hpp"
#include "sockets.hpp"

void Console::update() {
	// Reserve space at bottom for more elements
	static const float reservedSpace = -ImGui::GetFrameHeightWithSpacing();
	ImGui::BeginChild("ConsoleOutput", { 0, reservedSpace }, true, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4, 1 }); // Tighten line spacing

	// Add each item in the deque
	for (const std::string& i : _items) {
		// Set color of text
		ImVec4 color;
		bool hasColor = false;

		// std::string::starts_with() is introduced in C++20.
		if (i.starts_with("[ERROR]")) {
			// Error messages in red
			color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
			hasColor = true;
		} else if (i.starts_with("[INFO ]")) {
			// Information in yellow
			color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f);
			hasColor = true;
		}

		// Apply color if needed
		if (hasColor) ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::TextUnformatted(i.c_str());
		if (hasColor) ImGui::PopStyleColor();
	}

	// Scroll to end if autoscroll is set
	if (_scrollToEnd) {
		ImGui::SetScrollHereX(1.0f);
		ImGui::SetScrollHereY(1.0f);
		_scrollToEnd = false;
	}

	// Clean up console
	ImGui::PopStyleVar();
	ImGui::EndChild();

	// "Clear output" button
	if (ImGui::Button("Clear output")) clear();

	// Autoscroll checkbox
	ImGui::SameLine();
	ImGui::Checkbox("Autoscroll", &_autoscroll);
}

void Console::addText(const std::string& s) {
	// Don't add an empty string
	// (highly unlikely, but still check as string::back() called on an empty string throws a fatal exception)
	if (s.empty()) return;

	// Text goes on its own line if deque is empty or the last line ends with a \n
	if (_items.empty() || (_items.back().back() == '\n')) _items.push_back(s);
	else _items.back() += s; // Text goes on the last line (append)

	_scrollToEnd = _autoscroll; // Only force scroll if autoscroll is set first
}

void Console::forceNextLine() {
	// If the deque is empty, the item will have to be on its own line.
	if (!_items.empty()) {
		std::string& lastItem = _items.back();
		if (lastItem.back() != '\n') lastItem += '\n';
	}
}

void Console::clear() {
	_items.clear();
}

ClientWindow::ClientWindow(const DeviceData& data) {
	// Create the socket
	_sockfd = Sockets::createClientSocket(data);

	// Set title
	if (data.type == Bluetooth) title = std::format("Bluetooth Connection - {}", data.name);
	else title = std::format("{} Connection - {} port {}", connectionTypesStr[data.type], data.address, data.port);

	// Check socket validity
	if (_sockfd == INVALID_SOCKET) {
		// Socket invalid
		_output.addText("[ERROR] Socket creation failed.\n");
	} else {
		// Socket and connection valid
		_output.addText("[INFO ] Socket creation succeeded.\n");

		// Start the receiving background thread
		_recvThread = std::thread([&] {
			while (_sockfd != INVALID_SOCKET) {
				// Read data into buffer
				std::string recvBuf = "";
				int ret = Sockets::recvData(_sockfd, recvBuf);

				// Check status code
				if (ret == SOCKET_ERROR) {
					// Error, print error message
					socketErrorHandler();
					break;
				} else if (ret == 0) {
					// Server closed connection
					_output.forceNextLine();
					_output.addText("[INFO ] Remote host closed connection.\n");

					// Close the socket and exit the thread
					closesocket(_sockfd);
					_sockfd = INVALID_SOCKET;
					break;
				} else {
					// Receiving done successfully, add each line of the buffer into the deque
					size_t pos = 0;
					std::string sub = "";

					// Find each newline-delimited substring in the buffer to extract each individual line
					while ((pos = recvBuf.find('\n')) != std::string::npos) {
						pos++; // Increment position to include the newline in the substring
						sub = recvBuf.substr(0, pos); // Get the substring
						_output.addText(sub); // Add the substring to the output
						recvBuf.erase(0, pos); // Erase the substring from the buffer
					}

					// Add the last substring to the output (all other substrings have been erased from the buffer, the
					// only one left is the one after the last \n, or the whole string if there are no \n's present)
					_output.addText(recvBuf);
				}

				// Reset recv buffer
				recvBuf = "";
			}
		});
	}
}

ClientWindow::~ClientWindow() {
	if (_sockfd != INVALID_SOCKET) {
		// Shutdown socket to stop recv() in the parallel thread
		shutdown(_sockfd, SD_BOTH);

		// Close the socket
		closesocket(_sockfd);
	}

	// Join the receiving thread
	if (_recvThread.joinable()) _recvThread.join();
}

void ClientWindow::socketErrorHandler() {
	// Get numerical error code and string message
	auto [errorCode, errorMsg] = Sockets::getLastErr();
	if (errorCode == 0) return; // "[ERROR] 0: The operation completed successfully"

	// Socket errors are fatal, close the socket
	if (_sockfd != INVALID_SOCKET) {
		closesocket(_sockfd);
		_sockfd = INVALID_SOCKET;
	}

	// Add error line to console
	_output.forceNextLine();
	_output.addText(std::format("[ERROR] {}: {}\n", errorCode, errorMsg));
}

void ClientWindow::update() {
	ImGui::SetNextWindowSize({ 500, 300 }, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(title.c_str(), &open)) goto end;

	// Send textbox
	ImGui::SetNextItemWidth(-FLT_MIN); // Make the textbox full width
	if (ImGui::InputText("##input", &_sendBuf, ImGuiInputTextFlags_EnterReturnsTrue)) {
		// Add line ending
		const char* endings[] = { "", "\n", "\r", "\r\n" };

		// Send data and check if success
		if (Sockets::sendData(_sockfd, _sendBuf + endings[_currentLineEnding]) == SOCKET_ERROR) socketErrorHandler();

		_sendBuf = ""; // Blank out input textbox
		ImGui::SetItemDefaultFocus();
		ImGui::SetKeyboardFocusHere(-1); // Auto focus on input textbox
	}

	_output.update(); // Draw console output

	// Line ending combobox
	ImGui::SameLine();
	ImGui::SetNextItemWidth(150);
	ImGui::Combo("Line ending", &_currentLineEnding, "None\0Newline\0Carriage return\0Both\0");

	// End window
end:
	ImGui::End();
}
