// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector> // std::vector
#include <thread> // std::thread
#include <atomic> // std::atomic

#include "util.hpp"
#include "sockets.hpp"

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

	// Variables for sending (not declared static and put in update() because a static variable will propagate its
	// changes across all instances of ClientWindows, which is undesirable)
	std::string _sendBuf = ""; // Send buffer
	int _currentLineEnding = 0; // The index of the line ending selected in the combobox

	Console _output; // The output of the window, will hold system messages and data received from the server
	std::thread _recvThread; // Thread responsible for receiving data from the server

	/// <summary>
	/// Print the last socket error encountered by this object, then close the socket.
	/// </summary>
	void socketErrorHandler();

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
