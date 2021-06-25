// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector> // std::vector
#include <thread> // std::thread
#include <atomic> // std::atomic
#include <mutex> // std::mutex

#include "util.hpp"
#include "sockets.hpp"

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

class ClientWindow {
    std::atomic<SOCKET> _sockfd = INVALID_SOCKET; // Socket for connections
    std::atomic<bool> _connected = false; // If the window has an active connection
    Sockets::SocketError _lastRecvErr; // The last error encountered by the receiving thread

    Console _output; // The output of the window, will hold system messages and data received from the server
    std::string _sendBuf, _recvBuf; // Buffers
    int _currentLE = 0; // The index of the line ending selected in the combobox

    std::thread _recvThread; // Thread responsible for receiving data from the server
    std::mutex _recvAccess; // Mutex to ensure only one thread can access the recv buffer at a time
    int _receivedBytes = 0; // Number of bytes received from the socket
    std::atomic<bool> _recvNew = false; // If new data has been received

    /// <summary>
    /// Shut down and close the internal socket.
    /// </summary>
    void _closeConnection();

    /// <summary>
    /// Print the last socket error encountered by this object, then close the socket.
    /// </summary>
    void _errHandler();

    /// <summary>
    /// Print the error code and description of a given SocketError, then close the socket.
    /// </summary>
    /// <param name="err">A SocketError containing the error message to format and print</param>
    void _errHandler(const Sockets::SocketError& err);

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
    /// <param name="data">The DeviceData structure describing what to connect to</param>
    ClientWindow(const DeviceData& data);

    /// <summary>
    /// ClientWindow destructor, close the socket file descriptor.
    /// </summary>
    ~ClientWindow();

    /// <summary>
    /// Redraw the connection window and all of its elements, and sends data through the socket if when necessary.
    /// </summary>
    void update();
};
