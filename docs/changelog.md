# Network Socket Terminal Changelog

This document tracks the changes between Network Socket Terminal versions. Dates are written in the MM/DD/YYYY format.

## 0.2.0-alpha (Unreleased)

### Additions

- Added support for macOS. [IN PROGRESS]
- The "Show timestamps" feature is now available in connection windows on all platforms.
  - Timestamps are displayed in millisecond precision, with three digits instead of seven as in the previous release.
- Added a notification system to display application messages.
- Added a setting to control how many worker threads are spawned.
  - If `0` is specified, the app creates the maximum number of supported threads.
- Added a "send echoing" option to connection windows to show sent strings in the console.
- Added text selection to connection windows.
- Added a "View" menu to show/hide application windows.
- Added a "Connections" menu to list and focus on connection windows.

### Improvements

- Redesigned the loading spinner shown during an SDP inquiry.
- Initialization errors are now displayed in the notification area instead of as a corner overlay.
- The app now scales according to display DPI.
  - This improves rendering quality on Mac Retina screens.
- The app now bundles the Noto Sans Mono font, and support for loading additional fonts will be added in the future.

### Bug Fixes

- Fixed a crash when the app is closed without successful initialization of the thread pool.
- Fixed the reception buffer to correctly handle the ends of strings.
- Fixed the textbox autofocus in connection windows when the Enter key is pressed.
- Fixed an issue that would prevent sockets from closing if a connection window was closed while still connecting.
  - This also fixes a delayed crash on Windows occurring under the same scenario.
- Fixed possible hangs that would occur on socket errors.

### Removals

- Removed the "send textbox height" setting.
- Removed the "show FPS counter" setting.
- Removed the "use default font" setting.

### Development

- Switched the platform backend library to SDL.
- Switched the compiler on Linux to Clang.
- Added unit tests for socket functions.

## 0.1.0-alpha (10/05/2022)

- Initial release.
