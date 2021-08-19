// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm> // std::replace()
#include <chrono> // std::chrono
#include <mutex> // std::mutex

#include <imgui/imgui.h>

#include "connwindow.hpp"
#include "net/sockets.hpp"
#include "util/imguiext.hpp"
#include "util/formatcompat.hpp"

ConnWindow::operator bool() const {
    return _open;
}

bool ConnWindow::operator==(const std::string& s) const {
    return _title == s;
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

                // If the operation failed, save the last error
                // This makes it accessible in the main thread, where it can be printed.
                if (_receivedBytes == SOCKET_ERROR) _lastRecvErr = Sockets::getLastErr();

                // A non-positive integer means that the socket can no longer receive data so exit the thread
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
    if (err == NO_ERROR) return; // "[ERROR] 0: The operation completed successfully"

    // Socket errors are fatal
    _closeConnection();

    // Add error line to console
    Sockets::NamedError ne = Sockets::getErr(err);
    _output.addError(std::format("{} ({}): {}", ne.name, err, ne.desc));
}

void ConnWindow::_checkConnectionStatus() {
    if (_connAsync.ready()) {
        // If the socket is ready, get its file descriptor
        ConnectResult res = _connAsync.getValue();
        _sockfd = res.fd;

        if (_sockfd == INVALID_SOCKET) {
            // Something failed, print the error
            _errHandler(res.err);
        } else {
            // Connected, confirm success and start receiving data
            _connected = true;
            _output.addInfo("Done.");
            _startRecvThread();
        }
    } else {
        // Still connecting, display a message
        // We're using the AsyncFunction's user data variable as a flag to use to check if the "Connecting..." message
        // has been printed.
        if (!_connAsync.error() && !_connAsync.userData()) {
            _output.addInfo("Connecting...");
            _connAsync.userData() = true;
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
    if (!ImGui::Begin(_title.c_str(), &_open)) {
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
