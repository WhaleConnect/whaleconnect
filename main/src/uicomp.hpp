// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <future> // std::future, std::async()
#include <vector> // std::vector
#include <thread> // std::thread
#include <atomic> // std::atomic
#include <mutex> // std::mutex

#include <imgui/imgui.h>

#include "util.hpp"
#include "imguiext.hpp"
#include "sockets.hpp"

/// <summary>
/// A structure representing an item in a Console object's output.
/// </summary>
struct ConsoleItem {
    bool canUseHex; // If the item gets displayed as hexadecimal when the option is set
    std::string text; // The text of the item
    std::ostringstream textHex; // The text of the item, in hexadecimal format
    ImVec4 color; // The color of the item
    std::string timestamp; // The time when the item was added
};

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
/// Namespace containing helper functions to use with the UI component classes.
/// </summary>
namespace UIHelpers {
    /// <summary>
    /// Format a DeviceData instance into a readable title string.
    /// </summary>
    /// <param name="data">The DeviceData instance to format</param>
    /// <param name="useName">If the device's name should be substituted for its address for Bluetooth</param>
    /// <returns>The formatted string, see this function's definition for details</returns>
    std::string makeClientString(const DeviceData& data, bool useName);
}

/// <summary>
/// A class to represent a scrollable panel of output text with an input textbox.
/// </summary>
class Console {
    bool _scrollToEnd = false; // If the console is force-scrolled to the end
    bool _autoscroll = true; // If console autoscrolls when new data is put
    bool _showTimestamps = false; // If timestamps are shown in the output
    bool _showHex = false; // If items are shown in hexadecimal

    std::vector<ConsoleItem> _items; // Items in console output
    std::string _textBuf; // Buffer for the texbox
    int _currentLE = 0; // The index of the line ending selected

    /// <summary>
    /// Draw all elements below the textbox.
    /// </summary>
    void _updateOutput();

public:
    /// <summary>
    /// Draw the console output.
    /// </summary>
    /// <typeparam name="Fn">The function to call when the input textbox is activated</typeparam>
    /// <param name="fn">A function which takes a string which is the textbox contents at the time</param>
    template<class Fn>
    void update(Fn fn);

    /// <summary>
    /// Add text to the console. Does not make it go on its own line.
    /// </summary>
    /// <param name="s">The string to add to the output</param>
    /// <param name="color">The color of the text (optional, default is uncolored)</param>
    /// <param name="canUseHex">If the string gets displayed as hexadecimal when set (optional, default is true)</param>
    void addText(const std::string& s, ImVec4 color = {}, bool canUseHex = true);

    /// <summary>
    /// Add a red error message. Does make it go on its own line.
    /// </summary>
    /// <param name="s">The string to add to the output</param>
    void addError(const std::string& s);

    /// <summary>
    /// Add a yellow information message. Does make it go on its own line.
    /// </summary>
    /// <param name="s">The string to add to the output</param>
    void addInfo(const std::string& s);

    /// <summary>
    /// Add a newline to the last line of the output (if it doesn't already end with one). This causes the next item to
    /// go on a new line.
    /// </summary>
    void forceNextLine();

    /// <summary>
    /// Clear the console output.
    /// </summary>
    void clear();
};

template<class Fn>
void Console::update(Fn fn) {
    // Send textbox
    ImGui::SetNextItemWidth(-FLT_MIN); // Make the textbox full width
    if (ImGui::InputText("##ConsoleInput", &_textBuf, ImGuiInputTextFlags_EnterReturnsTrue)) {
        // Construct the string to send by adding the line ending to the end of the string
        const char* endings[] = { "", "\n", "\r", "\r\n" };
        std::string sendString = _textBuf + endings[_currentLE];

        if (sendString != "") fn(sendString);

        _textBuf = ""; // Blank out input textbox
        ImGui::SetItemDefaultFocus();
        ImGui::SetKeyboardFocusHere(-1); // Auto focus on input textbox
    }

    _updateOutput();
}

/// <summary>
/// A class to handle a connection in an easy-to-use GUI.
/// </summary>
class ConnWindow {
    std::atomic<SOCKET> _sockfd = INVALID_SOCKET; // Socket for connections
    std::future<SOCKET> _connFut; // The future object responsible for connecting asynchronously
    std::atomic<bool> _connected = false; // If the window has an active connection
    std::atomic<int> _lastConnectError = 0; // The last error encountered while connecting
    std::atomic<bool> _connectStop = false; // If the connection should be canceled
    bool _connectInitialized = false; // If the connecting async function has been started successfully
    bool _connectPrinted = false; // If the "Connecting..." message has been printed

    std::thread _recvThread; // Thread responsible for receiving data from the server
    std::mutex _recvAccess; // Mutex to ensure only one thread can access the recv buffer at a time
    int _receivedBytes = 0; // Number of bytes received from the socket
    std::atomic<bool> _recvNew = false; // If new data has been received
    int _lastRecvErr = 0; // The last error encountered by the receiving thread

    std::string _title; // Title of window
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
    std::string id; // Identifier of the window
    bool open = true; // If the window is open (affected by the close button)

    /// <summary>
    /// ConnWindow constructor, initialize a new object that can send/receive data across a socket file descriptor.
    /// </summary>
    /// <typeparam name="...Args">Additional arguments passed to the connector function</typeparam>
    /// <typeparam name="Fn">The connector function, see comment at the bottom of the file for details</typeparam>
    /// <param name="title">The title of the window, shown in the title bar</param>
    /// <param name="id">The unique identifier string for the window, made up of the address and port</param>
    /// <param name="fn">A function which connects and returns the socket fd and the last error code</param>
    /// <param name="...args">Additional arguments passed to the connector function</param>
    template<class Fn, class... Args>
    ConnWindow(const std::string& title, const std::string& id, Fn&& fn, Args&&... args);

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
ConnWindow::ConnWindow(const std::string& title, const std::string& id, Fn&& fn, Args&&... args)
    : _title(title), id(id) {
    // (The instantiation of _connFut is not put in the member initializer list because of its large size.)
    try {
        _connFut = std::async(std::launch::async, [&](Args&&... _args) {
            // Call the provided function with the stop flag and additional arguments
            ConnectResult ret = fn(_connectStop, _args...);

            // Get the last connect error
            _lastConnectError = ret.err;

            // Return the socket file descriptor from the async function
            return ret.fd;
        }, args...);
        _connectInitialized = true;
    } catch (const std::system_error&) {
        // Failed to start the function - usually because something happened in the system.
        _output.addError("System error - Failed to start connecting.");
    }
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
//
// It is desirable to add a small delay right before the return to prevent the function from completing too fast for
// the ConnWindow to process. [Use `std::this_thread::sleep_for()`]
