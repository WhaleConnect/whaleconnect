// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// A class to handle a socket connection in a GUI window

#pragma once

#include <string>
#include <functional> // std::bind()
#include <string_view>

#include "console.hpp"
#include "async/async.hpp"
#include "net/sockets.hpp"

/// <summary>
/// A class to handle a connection in an easy-to-use GUI.
/// </summary>
class ConnWindow {
    Sockets::Socket _socket; // Socket for connection
    bool _connected = false; // If the socket has an active connection to a server

    std::string _title; // Title of window
    std::string _windowText; // The text in the window's title bar
    bool _open = true; // If the window is open (affected by the close button)
    Console _output{ std::bind(&ConnWindow::_sendHandler, this, std::placeholders::_1) }; // Console with input textbox

    /// <summary>
    /// Connect to a server.
    /// </summary>
    /// <param name="data">The server to connect to</param>
    /// <returns>A void task</returns>
    Task<> _connect(const Sockets::DeviceData& data);

    /// <summary>
    /// Send a string through the socket if it's connected.
    /// </summary>
    /// <param name="s">The string to send</param>
    /// <returns>A void task</returns>
    Task<> _sendHandler(std::string_view s);

    /// <summary>
    /// Function callback to execute when the socket has an input event.
    /// </summary>
    /// <returns>A void task</returns>
    Task<> _readHandler();

    /// <summary>
    /// Print the error (if any) from a function that may have failed.
    /// </summary>
    /// <typeparam name="T">The type of the value stored in the return object</typeparam>
    /// <param name="returnCode">The returned object from the function to test for</param>
    template <class T>
    void _errorHandler(const System::MayFail<T>& returnCode) {
        // Check for non-fatal errors, then add error line to console
        if (!returnCode) _output.addError(System::formatErr(returnCode.error()));
    }

public:
    /// <summary>
    /// ConnWindow constructor, set the window title and create the connection.
    /// </summary>
    /// <param name="data">The server to connect to</param>
    /// <param name="title">Text to display in the window's titlebar</param>
    /// <param name="extraInfo">Extra information to display in the window's titlebar</param>
    ConnWindow(const Sockets::DeviceData& data, std::string_view title, std::string_view extraInfo);

    /// <summary>
    /// Get the window's title.
    /// </summary>
    /// <returns>The window's title</returns>
    std::string_view getTitle() const { return _title; }

    /// <summary>
    /// Get the internal socket descriptor.
    /// </summary>
    /// <returns>The file descriptor of the managed socket</returns>
    SOCKET getSocket() const { return _socket.get(); }

    /// <summary>
    /// Check if the window is open.
    /// </summary>
    bool open() const { return _open; }

    /// <summary>
    /// Redraw the connection window and send data through the socket.
    /// </summary>
    void update();
};
