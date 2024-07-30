# WhaleConnect Changelog

This document tracks the changes between WhaleConnect versions. Dates are written in the MM/DD/YYYY format.

## 1.0.1 (07/29/2024)

### Bug Fixes

- Fixed an occasional crash occurring on macOS when a client connection with pending I/O was closed.
- Fixed an infinite connection loop in TLS clients if the server disconnected without sending data.
- Fixed a crash on macOS when using Bluetooth features.
- Updated the minimum version of macOS to 13.3.

## 1.0.0 (06/30/2024)

This project was rebranded to WhaleConnect in this release.

### Improvements

- Improved thread safety on Windows.
- Prevented a scrollbar from occasionally showing in the "New Connection > Internet Protocol" tab.
- Prevented the last digit in the timestamp display from getting clipped.
- Prevented receive and accept operations from continuing after an error.

### Bug Fixes

- Fixed a crash occurring when there is an error during server creation.

## 0.4.0-beta (06/25/2024)

### Additions

- Added file-based configuration to allow changing settings at runtime.
- Added a settings editor window.
- Added a "Bluetooth UUIDs" setting to add/remove UUIDs in SDP queries.
- Added a "Quit" option under the "File" menu item.
- Added a "Help" menu item with build information.
- Added the ability to use system menu bars on macOS.
- Added dynamic scrolling speed when selecting text.
  - The text will scroll faster as the cursor moves farther from the window boundary.

### Improvements

- Added a permanent horizontal scrollbar in connection windows to prevent flickering while scrolling.
- Updated the timestamps display in connection windows to reduce the number of computations per application frame.
- Updated error reporting to be in terms of location in source files.
- Updated the receive buffer length input to not take effect when the user is making modifications.
- Updated the initialization of worker threads to better reflect hardware concurrency.
  - The number of worker threads created is 1 less than the specified number to account for the main application thread which now also runs an I/O event loop.

### Bug Fixes

- Fixed a crash in connection windows if text was highlighted and the output was cleared.
- Fixed the placement of notification close buttons when the notifications window has a vertical scrollbar.

## 0.3.0-alpha (01/03/2024)

### Additions

- Added server functionality for Internet and Bluetooth sockets.
- Added TLS support for TCP client sockets.
- Added a setting to change the number of entries in io_uring instances.
- Added a connection window option to change the receive buffer length.

### Improvements

- Prevented multiple close calls on invalid or already closed sockets.
- Increased the reliability of IP address lookup for client connections.
  - If the app receives multiple addresses for a host, it tries connecting to each to increase the chance of a successful connection.
- Improved thread safety.
  - This fixed intermittent data loss in receive operations.

### Bug Fixes

- Fixed an issue where receive operations would not continue after an error.
- Fixed text selection in connection windows with lots of lines.
- Fixed copying text in connection windows if a selection was made on one line dragging right to left.
- Fixed a regression in v0.2.0 where events on Bluetooth client sockets on Windows would not complete.
- Fixed a handle leak occurring after listing paired Bluetooth devices on Windows.
- Fixed SDP queries on macOS 12 (Monterey) and later.
- Fixed duplicate entries in the macOS paired devices list resulting from a device being paired multiple times.

### Removals

- Removed L2CAP stream and datagram options due to their infrequent usage.

## 0.2.2-alpha (08/13/2023)

### Improvements

- Bundled all libraries with the app package to prevent dependency mismatches.
- Moved the license documents into the Resources directory in the macOS app bundle.

## 0.2.1-alpha (08/12/2023)

### Development

- Adjusted the xmake build scripts to create distributable packages.

## 0.2.0-alpha (08/12/2023)

### Additions

- Added support for macOS.
- Added the "Show timestamps" feature in connection windows on all platforms.
  - Timestamps are displayed in millisecond precision with three digits.
- Added scaling based on display DPI. This improves rendering quality on Mac Retina screens.
- Added a notification system to display application messages.
- Added a setting to control how many worker threads are spawned.
  - If `0` is specified, the app creates the maximum number of supported threads.
- Added a "send echoing" option to connection windows to show sent strings in the console.
- Added hostname entry and DNS lookup in new IP connections.
- Added text selection to connection windows.
- Added a "View" menu to show/hide application windows.
- Added a "Connections" menu to list and focus on connection windows.

### Improvements

- Redesigned the loading spinner shown during an SDP inquiry.
- Updated initialization errors to display as a notification instead of a corner overlay.
- Bundled the Noto Sans Mono font with the app. Support for loading additional fonts will be added in a future release.
  - **Update:** Loading fonts is now supported in v0.4.0.
- Removed synchronization between worker threads to improve performance.

### Bug Fixes

- Fixed a crash when the app is closed without successful initialization of the thread pool.
- Fixed the reception buffer to correctly handle the ends of strings.
- Fixed the textbox autofocus in connection windows when the Enter key is pressed.
- Fixed an issue that would prevent sockets from closing if a connection window was closed while still connecting.
  - This fixed a delayed crash on Windows occurring under the same scenario.
- Fixed possible hangs that would occur on socket errors.

### Removals

- Removed the "send textbox height" setting.
- Removed the "show FPS counter" setting.
- Removed the "use default font" setting.

## 0.1.0-alpha (10/05/2022)

Initial release of WhaleConnect.
