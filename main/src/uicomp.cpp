// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm> // std::replace()
#include <mutex> // std::mutex
#include <chrono> // std::chrono::microseconds, std::chrono::system_clock::now()

#include <imgui/imgui.h>

#include "error.hpp"
#include "uicomp.hpp"
#include "imguiext.hpp"
#include "util.hpp"
#include "sockets.hpp"
#include "formatcompat.hpp"

std::string UIHelpers::makeClientString(const DeviceData& data, bool useName) {
    // Connection type
    const char* type = connectionTypesStr[data.type];

    // Bluetooth connections are described using the device's name (e.g. "MyESP32"),
    // IP connections (TCP/UDP) use the device's IP address (e.g. 192.168.0.178).
    std::string deviceString = ((data.type == Bluetooth) && useName) ? data.name : data.address;

    // Newlines may be present in a Bluetooth device name, and if they get into a window's title, anything after the
    // first one will get cut off (the title bar can only hold one line). Replace them with spaces to keep everything on
    // one line.
    if (useName) std::replace(deviceString.begin(), deviceString.end(), '\n', ' ');

    // Format the values into a string and return it
    return std::format("{} Connection - {} port {}##{}", type, deviceString, data.port, data.address);
}

void Console::_updateOutput() {
    // Reserve space at bottom for more elements
    static const float reservedSpace = -ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ConsoleOutput", { 0, reservedSpace }, true, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4, 1 }); // Tighten line spacing

    // Add each item in the deque
    for (const auto& i : _items) {
        // Only color tuples with the last value set to 1 are considered
        bool hasColor = (i.color.w == 1.0f);

        // Timestamps
        if (_showTimestamps) {
            ImGui::TextUnformatted(i.timestamp);
            ImGui::SameLine();
        }

        // Apply color if needed
        if (hasColor) ImGui::PushStyleColor(ImGuiCol_Text, i.color);
        ImGui::TextUnformatted((_showHex && i.canUseHex) ? i.textHex.str() : i.text);
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

    // "Options" button
    ImGui::SameLine();
    if (ImGui::Button("Options...")) ImGui::OpenPopup("options");

    // Popup for more options
    if (ImGui::BeginPopup("options")) {
        ImGui::MenuItem("Autoscroll", nullptr, &_autoscroll);

        // TODO: Remove the #if check once all major compilers implement timezones for <chrono>
#if __cpp_lib_chrono >= 201803L
        ImGui::MenuItem("Show timestamps", nullptr, &_showTimestamps);
#endif

        ImGui::MenuItem("Show hexadecimal", nullptr, &_showHex);

        ImGui::Separator();

        ImGui::MenuItem("Clear texbox on send", nullptr, &_clearTextboxOnSend);

        ImGui::EndPopup();
    }

    // Line ending combobox
    ImGui::SameLine();

    // The code used to calculate where to put the combobox is derived from
    // https://github.com/ocornut/imgui/issues/4157#issuecomment-843197490
    float comboWidth = 150.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - comboWidth));

    ImGui::SetNextItemWidth(comboWidth);
    ImGui::Combo("##lineEnding", &_currentLE, "None\0Newline\0Carriage return\0Both\0");
}

void Console::addText(const std::string& s, ImVec4 color, bool canUseHex) {
    // Don't add an empty string
    // (highly unlikely, but still check as string::back() called on an empty string throws a fatal exception)
    if (s.empty()) return;

    // Get timestamp
    // TODO: Remove the #if check once all major compilers implement timezones for <chrono>
#if __cpp_lib_chrono >= 201803L
    using namespace std::chrono;

    // TODO: Limitations with {fmt} - `.time_since_epoch()` is added. Remove when standard std::format() is used
    std::string timestamp = std::format("{:%T} >", current_zone()->to_local(system_clock::now()).time_since_epoch());
#endif

    // Text goes on its own line if deque is empty or the last line ends with a newline
    if (_items.empty() || (_items.back().text.back() == '\n')) _items.push_back({ canUseHex, s, {}, color, timestamp });
    else _items.back().text += s; // Text goes on the last line (append)

    // The code block below is located here as a "caching" mechanism - the hex representation is only computed when the
    // item is added. It is simply retreived in the main loop.
    // If this was in `update()` (which would make the code slightly simpler), this would get unnecessarily re-computed
    // every frame - 60 times a second or more.
    if (canUseHex) {
        // Build the hex representation of the string
        // We're appending to the last item's ostringstream (`_items.back()` is guaranteed to exist at this point).
        // We won't need to check for newlines using this approach.
        std::ostringstream& oss = _items.back().textHex;
        for (const unsigned char& c : s) {
            oss << std::hex            // Express integers in hexadecimal (i.e. base-16)
                << std::uppercase      // Make hex digits A-F uppercase
                << std::setw(2)        // Hex tuples (groups of digits) always contain 2 digits
                << std::setfill('0')   // If something is less than 2 digits it's padded with 0s (i.e. "A" => "0A")
                << static_cast<int>(c) // Use the above formatting rules to append the character's codepoint
                << " ";                // Single space between tuples to separate them
        }
    }

    _scrollToEnd = _autoscroll; // Only force scroll if autoscroll is set first
}

void Console::addError(const std::string& s) {
    // Error messages in red
    forceNextLine();
    addText(std::format("[ERROR] {}\n", s), { 1.0f, 0.4f, 0.4f, 1.0f }, false);
}

void Console::addInfo(const std::string& s) {
    // Information in yellow
    forceNextLine();
    addText(std::format("[INFO ] {}\n", s), { 1.0f, 0.8f, 0.6f, 1.0f }, false);
}

void Console::forceNextLine() {
    // If the deque is empty, the item will have to be on its own line.
    if (_items.empty()) return;

    std::string& lastItem = _items.back().text;
    if (lastItem.back() != '\n') lastItem += '\n';
}

void Console::clear() {
    _items.clear();
}

ConnWindow::~ConnWindow() {
    _closeConnection();

    // Join the receiving thread
    if (_recvThread.joinable()) _recvThread.join();
}

void ConnWindow::_startRecvThread() {
    // Receiving loop function, this runs in the background of a ConnWindow
    auto recvFunc = [&] {
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

                // If the operation failed, save the last error because WSAGetLastError()/errno does not propagate
                // across threads (it is thread local)
                // This makes it accessible in the main thread, where it can be printed.
                if (_receivedBytes == SOCKET_ERROR) _lastRecvErr = Sockets::getLastErr();

                // A non-positive integer means that the socket can no longer receive data so exit the thread:
                if (_receivedBytes <= 0) break;
            }
        }
    };

    try {
        // Start the thread with the receiving loop
        _recvThread = std::thread(recvFunc);
    } catch (const std::system_error&) {
        // Failed to start the thread
        // This doesn't mean the connection is useless - sending data can still be done.
        _output.addError("System error - Failed to start the receiving thread. (You may still send data.)");
    }
}

void ConnWindow::_closeConnection() {
    Sockets::destroySocket(_sockfd);
    _connected = false;
    _connectStop = true;
}

void ConnWindow::_errHandler(int err) {
    if (err == 0) return; // "[ERROR] 0: The operation completed successfully"

    // Socket errors are fatal
    _closeConnection();

    // Add error line to console
    Sockets::NamedError ne = Sockets::getErr(err);
    _output.addError(std::format("{} ({}): {}", ne.name, err, ne.desc));
}

void ConnWindow::_checkConnectionStatus() {
    // The && should short-circuit here - wait_for() should not execute if valid() returns false.
    // This is the desired and expected behavior, since waiting on an invalid future throws an exception.
    if (_connFut.valid() && (_connFut.wait_for(std::chrono::microseconds(0)) == std::future_status::ready)) {
        // If the socket is ready, get its file descriptor
        _sockfd = _connFut.get();

        if (_sockfd == INVALID_SOCKET) {
            // Something failed, print the error
            _errHandler(_lastConnectError);
        } else {
            // Connected, confirm success and start receiving data
            _connected = true;
            _output.addInfo("Done.");
            _startRecvThread();
        }
    } else {
        // Still connecting, display a message
        if (_connectInitialized && !_connectPrinted) {
            _output.addInfo("Connecting...");
            _connectPrinted = true;
        }
    }
}

void ConnWindow::_updateOutput() {
    // Check status code
    switch (_receivedBytes) {
    case SOCKET_ERROR:
        // Error, print message
        _errHandler(_lastRecvErr);
        break;
    case 0:
        // Peer closed connection
        _output.addInfo("Remote host closed connection.");
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

void ConnWindow::update() {
    ImGui::SetNextWindowSize({ 500, 300 }, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(_title.c_str(), &open)) {
        ImGui::End();
        return;
    }

    _checkConnectionStatus(); // Check the status of the async connection

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

    // Draw console output
    _output.update([&](const std::string& text) {
        if (_connected) {
            int ret = Sockets::sendData(_sockfd, text);
            if (ret == SOCKET_ERROR) _errHandler(Sockets::getLastErr());
        } else {
            _output.addInfo("The socket is not connected.");
        }
    });

    // End window
    ImGui::End();
}
