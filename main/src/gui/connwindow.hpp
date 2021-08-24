// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A class to handle a socket connection in a GUI window

#pragma once

#include <chrono> // std::chrono
#include <thread> // std::thread
#include <atomic> // std::atomic
#include <mutex> // std::mutex

#include <imgui/imgui.h>

#include "console.hpp"
#include "net/sockets.hpp"
#include "util/imguiext.hpp"
#include "util/asyncfunction.hpp"

/// <summary>
/// A structure representing the result of a connection attempt.
/// </summary>
/// <remarks>
/// This structure contains a SOCKET to hold the file descriptor of the connection and an int to hold the last error
/// code that was caught while connecting.
/// </remarks>
struct ConnectResult {
    SOCKET fd = INVALID_SOCKET; // The resultant file descriptor
    int err = NO_ERROR; // Any error that occurred
};

/// <summary>
/// A class to handle a connection in an easy-to-use GUI.
/// </summary>
class ConnWindow {
    std::atomic<SOCKET> _sockfd = INVALID_SOCKET; // Socket for connections
    AsyncFunction<ConnectResult, bool> _connAsync; // Asynchronous connection function
    std::atomic<bool> _connected = false; // If the window has an active connection
    std::atomic<bool> _connectStop = false; // If the connection should be canceled

    std::thread _recvThread; // Thread responsible for receiving data from the server
    std::mutex _recvAccess; // Mutex to ensure only one thread can access the recv buffer at a time
    int _receivedBytes = 0; // Number of bytes received from the socket
    std::atomic<bool> _recvNew = false; // If new data has been received
    int _lastRecvErr = 0; // The last error encountered by the receiving thread

    const std::string _title; // Title of window
    bool _open = true; // If the window is open (affected by the close button)
    Console _output; // The output of the window, will hold system messages and data received from the server
    std::string _recvBuf; // Receive buffer

    /// <summary>
    /// Start the receiving background thread.
    /// </summary>
    void _startRecvThread();

    /// <summary>
    /// Shut down and close the internal socket.
    /// </summary>
    void _closeConnection();

    /// <summary>
    /// Print the error code and description of a given SocketError, then close the socket.
    /// </summary>
    /// <param name="err">The error code to format and print</param>
    void _errHandler(int err);

    /// <summary>
    /// Check if the async connection finished and what errors occured, then perform the appropriate behavior.
    /// </summary>
    void _checkConnectionStatus();

    /// <summary>
    /// Add text to the output. Called after receiving data from the socket.
    /// </summary>
    void _updateOutput();

public:
    /// <summary>
    /// ConnWindow constructor, initialize a new object that can send/receive data across a socket file descriptor.
    /// </summary>
    /// <typeparam name="...Args">Additional arguments passed to the connector function</typeparam>
    /// <typeparam name="Fn">The connector function, see comment at the bottom of the file for details</typeparam>
    /// <param name="title">The title of the window, shown in the title bar</param>
    /// <param name="fn">A function which connects and returns the socket fd and the last error code</param>
    /// <param name="...args">Additional arguments passed to the connector function</param>
    template<class Fn, class... Args>
    ConnWindow(const std::string& title, Fn&& fn, Args&&... args);

    /// <summary>
    /// Check if the window is open. Returns the internal `_open` member.
    /// </summary>
    operator bool() const;

    /// <summary>
    /// ConnWindow destructor, close the socket file descriptor.
    /// </summary>
    ~ConnWindow();

    /// <summary>
    /// Redraw the connection window and send data through the socket.
    /// </summary>
    void update();
};

template<class Fn, class... Args>
ConnWindow::ConnWindow(const std::string& title, Fn&& fn, Args&&... args) : _title(title) {
    _connAsync.run([&, args...]() -> ConnectResult {
        // Small delay to prevent the function from finishing too fast
        std::this_thread::sleep_for(std::chrono::microseconds(100));

        // Call the provided function with the stop flag and additional arguments
        return fn(_connectStop, args...);
    });

    // Failed to start the function - usually because something happened in the system.
    if (_connAsync.error()) _output.addError("System error - Failed to start connecting.");
}

// The ConnWindow connector function:
//
// A function (either a lambda or traditionally-declared) should be passed to the `fn` parameter. This is known as the
// connector function.
//
// The exact code for establishing the connection should be contained within the body of this function.
// This is why it is called a "connector function" - it handles the connection process, the ConnWindow doesn't.
//
// This function should return a ConnectResult. See its documentation comment for details about what it holds.
//
// This function must also take a `const std::atomic<bool>&` as its very first argument. This is the stop signal used
// to abort a connection and is set to true when the ConnWindow is closed. This does not have to be used.
//
// This function may take additional arguments after the stop signal. These are fed in via the `args` parameter of the
// ConnWindow constructor.
//
// Therefore, the minimal definition for a valid connector function is as follows:
//
//     ConnectResult myConnFunc(const std::atomic<bool>& sig);
//
// While the body of the function may be structured as:
//
//     SOCKET sockfd = createSomeSocket();
//     return { sockfd, Sockets::getLastErr() };
