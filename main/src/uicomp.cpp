// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm> // std::replace()
#include <format> // std::format() [C++20]
#include <mutex> // std::mutex

#include <imgui/imgui.h>

#include "uicomp.hpp"
#include "imguiext.hpp"
#include "util.hpp"
#include "sockets.hpp"

std::string UIHelpers::makeClientWindowTitle(const DeviceData& data) {
	// Connection type
	const char* connectionType = connectionTypesStr[data.type];

	// Bluetooth connections are described using the device's name (e.g. "MyESP32"),
	// IP connections (TCP/UDP) use the device's IP address (e.g. 192.168.0.178).
	std::string deviceString = (data.type == Bluetooth) ? data.name : data.address;

	// Newlines may be present in a Bluetooth device name, and if they get into a window's title, anything after the
	// first one will get cut off (the title bar can only hold one line). Replace them with spaces to keep everything on
	// one line.
	std::replace(deviceString.begin(), deviceString.end(), '\n', ' ');

	// Format the values into a string and return it
	return std::format("{} Connection - {} port {}", connectionType, deviceString, data.port);
}

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
			color = { 1.0f, 0.4f, 0.4f, 1.0f };
			hasColor = true;
		} else if (i.starts_with("[INFO ]")) {
			// Information in yellow
			color = { 1.0f, 0.8f, 0.6f, 1.0f };
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
	title = UIHelpers::makeClientWindowTitle(data);

	if (_sockfd == INVALID_SOCKET) _output.addText("[ERROR] Socket creation failed.\n"); // Socket invalid
	else _connected = true; // Socket and connection valid

	// Start the receiving background thread
	// (If the socket creation failed, `_connected` remains false and the thread simply exits right away.)
	_recvThread = std::thread([&] {
		// Read data into buffer as long as the socket is connected
		while (_connected) {
			// The `recvNew` flag is used to check if there's new data not yet added to the output. If there is, this
			// thread will not receive any more until the flag is set back to false by the main thread. This is to
			// prevent losing data from the main thread not looping fast enough.
			if (!_recvNew) {
				// Create a lock_guard to restrict access to the receive buffer
				std::lock_guard<std::mutex> guard(_recvAccess);

				// Receive the data from the socket, keep track of the number of bytes and the incoming string
				_receivedBytes = Sockets::recvData(_sockfd, _recvBuf);

				// There is now new data:
				_recvNew = true;

				// -1 (SOCKET_ERROR) and 0 (disconnect) mean that this thread should be closed because the socket
				// will no longer be able to receive more data.
				if (_receivedBytes <= 0) break;
			}
		}
	});
}

ClientWindow::~ClientWindow() {
	_closeConnection();

	// Join the receiving thread
	if (_recvThread.joinable()) _recvThread.join();
}

void ClientWindow::_closeConnection() {
	if (_connected) {
		_connected = false;

		// Shutdown socket to stop recv() in the parallel thread
		shutdown(_sockfd, SD_BOTH);

		// Close the socket
		closesocket(_sockfd);
	}
}

void ClientWindow::_errHandler() {
	// Get numeric error code and string message
	auto [errorCode, errorMsg] = Sockets::getLastErr();
	if (errorCode == 0) return; // "[ERROR] 0: The operation completed successfully"

	// Socket errors are fatal
	_closeConnection();

	// Add error line to console
	_output.forceNextLine();
	_output.addText(std::format("[ERROR] {}: {}\n", errorCode, errorMsg));
}

void ClientWindow::_updateOutput() {
	// Check status code
	switch (_receivedBytes) {
	case SOCKET_ERROR:
		// Error, print message
		_errHandler();
		break;
	case 0:
		// Peer closed connection
		_output.forceNextLine();
		_output.addText("[INFO ] Remote host closed connection.\n");
		_closeConnection();
		break;
	default:
		// Receiving done successfully, add each line of the buffer into the vector
		size_t pos = 0;
		std::string sub = "";

		// Find each newline-delimited substring in the buffer to extract each individual line
		while ((pos = _recvBuf.find('\n')) != std::string::npos) {
			pos++; // Increment position to include the newline in the substring
			sub = _recvBuf.substr(0, pos); // Get the substring
			_output.addText(sub); // Add the substring to the output
			_recvBuf.erase(0, pos); // Erase the substring from the buffer
		}

		// Add the last substring to the output (all other substrings have been erased from the buffer, the only one
		// left is the one after the last \n, or the whole string if there are no \n's present)
		_output.addText(_recvBuf);
	}

	// Reset recv buffer
	_recvBuf = "";
}

void ClientWindow::update() {
	ImGui::SetNextWindowSize({ 500, 300 }, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(title.c_str(), &open)) {
		ImGui::End();
		return;
	}

	// Send textbox
	ImGui::SetNextItemWidth(-FLT_MIN); // Make the textbox full width
	if (ImGui::InputText("##input", &_sendBuf, ImGuiInputTextFlags_EnterReturnsTrue)) {
		// Add line ending
		const char* endings[] = { "", "\n", "\r", "\r\n" };

		// Send data and check if success
		if (_connected && (Sockets::sendData(_sockfd, _sendBuf + endings[_currentLE]) == SOCKET_ERROR)) _errHandler();

		_sendBuf = ""; // Blank out input textbox
		ImGui::SetItemDefaultFocus();
		ImGui::SetKeyboardFocusHere(-1); // Auto focus on input textbox
	}

	if (_connected && _recvNew) {
		// If the socket is connected and there is new data received, try to lock the receiving access mutex:
		std::unique_lock<std::mutex> lock(_recvAccess, std::try_to_lock);

		if (lock.owns_lock()) {
			// Add the appropriate text to the output (error, "closed connection" message, or the stuff received)
			_updateOutput();

			// If the lock was successful, set `_recvNew` to false to indicate the output is up to date.
			_recvNew = false;
		}
	}

	_output.update(); // Draw console output

	// Line ending combobox
	ImGui::SameLine();

	// The code used to calculate where to put the combobox is derived from
	// https://github.com/ocornut/imgui/issues/4157#issuecomment-843197490
	float comboWidth = 150.0f;
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - comboWidth));

	ImGui::SetNextItemWidth(comboWidth);
	ImGui::Combo("##lineEnding", &_currentLE, "None\0Newline\0Carriage return\0Both\0");

	// End window
	ImGui::End();
}
