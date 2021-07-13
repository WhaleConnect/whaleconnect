// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <future> // std::future, std::async()
#include <vector> // std::vector
#include <thread> // std::thread
#include <atomic> // std::atomic
#include <mutex> // std::mutex
#include <utility> // std::pair

#include "util.hpp"
#include "sockets.hpp"

typedef std::pair<SOCKET, int> ConnectResult;

/// <summary>
/// Namespace containing helper functions to use with the UI component classes.
/// </summary>
namespace UIHelpers {
    /// <summary>
    /// Format a DeviceData instance into a string.
    /// </summary>
    /// <param name="data">The DeviceData instance to format</param>
    /// <returns>The formatted string, see this function's declaration for details.</returns>
    std::string makeClientWindowTitle(const DeviceData& data);
}

/// <summary>
/// A class to represent a scrollable panel of output text.
/// </summary>
class Console {
    bool _scrollToEnd = false; // If the console is force-scrolled to the end
    bool _autoscroll = true; // If console autoscrolls when new data is put
    std::vector<std::string> _items; // Items in console output

public:
    /// <summary>
    /// Redraw the console output. Must be called after adding an item to make it show up.
    /// </summary>
    void update();

    /// <summary>
    /// Add text to the console. Does not make it go on its own line.
    /// </summary>
    /// <param name="s">The string to add to the output</param>
    void addText(const std::string& s);

    /// <summary>
    /// Add a newline to the last line of the output (if it doesn't already end with one).
    /// </summary>
    void forceNextLine();

    /// <summary>
    /// Clear the console output by clearing the items deque.
    /// </summary>
    void clear();
};

/// <summary>
/// A class to handle a connection in an easy-to-use GUI.
/// </summary>
class ClientWindow {
    std::atomic<SOCKET> _sockfd = INVALID_SOCKET; // Socket for connections
    std::future<SOCKET> _connFut; // The future object responsible for connecting asynchronously
    std::atomic<bool> _connected = false; // If the window has an active connection
    std::atomic<int> _lastConnectError = 0; // The last error encountered while connecting
    bool _connectInitialized = false; // If the "Connecting..." message has been printed
    std::atomic<bool> _connectStop = false; // If the connection should be canceled

    std::thread _recvThread; // Thread responsible for receiving data from the server
    std::mutex _recvAccess; // Mutex to ensure only one thread can access the recv buffer at a time
    int _receivedBytes = 0; // Number of bytes received from the socket
    std::atomic<bool> _recvNew = false; // If new data has been received
    int _lastRecvErr = 0; // The last error encountered by the receiving thread

    Console _output; // The output of the window, will hold system messages and data received from the server
    std::string _sendBuf, _recvBuf; // Buffers
    int _currentLE = 0; // The index of the line ending selected in the combobox

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
    std::string title; // Title of window
    bool open = true; // If the window is open (affected by the close button)

    /// <summary>
    /// ClientWindow constructor, connect to the address and port specified.
    /// </summary>
    /// <typeparam name="...Args">Additional arguments passed to the connector function</typeparam>
    /// <typeparam name="Fn">The connector function, see comment at the bottom of the file for details</typeparam>
    /// <param name="name">The title of the window</param>
    /// <param name="fn">A function which connects and returns the socket fd and the last error code</param>
    /// <param name="...args">Additional arguments passed to the connector function</param>
    template<class Fn, class... Args>
    ClientWindow(const std::string& name, Fn&& fn, Args&&... args);

    /// <summary>
    /// ClientWindow destructor, close the socket file descriptor.
    /// </summary>
    ~ClientWindow();

    /// <summary>
    /// Redraw the connection window and send data through the socket.
    /// </summary>
    void update();
};

template<class Fn, class... Args>
ClientWindow::ClientWindow(const std::string& name, Fn&& fn, Args&&... args) : title(name) {
    // (The instantiation of _connFut is not put in the member initializer list because of its large size.)
    _connFut = std::async(std::launch::async, [&](Args&&... _args) {
        // Call the provided function with the stop flag and additional arguments
        ConnectResult ret = fn(_connectStop, _args...);

        // Get the last connect error
        _lastConnectError = ret.second;

        // Return the socket file descriptor from the async function
        return ret.first;
    }, args...);
}

// The ClientWindow connector function:
//
// A function (either a lambda or traditionally-declared) should be passed to the `fn` parameter. This is known as the
// connector function.
//
// The exact code for establishing the connection should be contained within the body of this function.
// This is why it is called a "connector function" - it handles the connection process, the ClientWindow doesn't.
//
// This function should return a ConnectResult, which contains an int and a SOCKET in that order. The int is the error
// code from connecting [use Sockets::getLastError()] and the SOCKET is the file descriptor of the new connection
// (INVALID_SOCKET is allowed).
//
// This function must also take a `const std::atomic<bool>&` as its very first argument. This is the stop signal used
// to abort a connection and is set to true when the ClientWindow is closed. This does not have to be used.
//
// This function may take additional arguments after the stop signal. These are fed in via the `args` parameter of the
// ClientWindow constructor.
//
// Therefore, the minimal definition for a valid connector function is as follows:
//
//     ConnectResult myConnFunc(const std::atomic<bool>& sig);
//
// While the body of the function may be structured as:
//
//     SOCKET sockfd = createSomeSocket();
//     return { sockfd, Sockets::getLastErr() };
//
// It is desirable to add a small delay right before the return to prevent the function from completing too fast for
// the ClientWindow to process. [Use `std::this_thread::sleep_for()`]
