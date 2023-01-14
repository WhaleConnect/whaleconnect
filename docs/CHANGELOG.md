# Network Socket Terminal Changelog

This document tracks the changes between Network Socket Terminal versions. Dates are written in the MM/DD/YYYY format.

## 0.2.0-alpha (Unreleased)

### Additions

- Added support for macOS. [IN PROGRESS]
- Added a "Copy line" right-click option in connection windows.
- The "Show timestamps" feature is now available in connection windows on all platforms.
  - Timestamps are displayed in millisecond precision, with three digits instead of seven as in the previous release.
- Added a notification system to display application messages.
  - The notification area is shown on the right side of the app by default, but it can be moved and docked.
  - A notification can close automatically after a few seconds; this can be canceled by hovering over its close button.
- Added a setting to control how many worker threads are spawned.
  - If `0` is specified, the app creates the maximum number of supported threads.

### Improvements

- Redesigned the loading spinner shown during an SDP inquiry.
- Initialization errors are now displayed in the notification area instead of as a corner overlay.

### Bug Fixes

- Fixed a crash when the app is closed without successful initialization of the thread pool.
- Fixed the reception buffer to correctly handle the ends of strings.
- Fixed the textbox autofocus in connection windows when the Enter key is pressed.
- Fixed an issue that would prevent sockets from closing if a connection window was closed while still connecting.
  - This also fixes a delayed crash on Windows occurring under the same scenario.

### Removals

- Removed the "send textbox height" setting.
- Removed the "show FPS counter" setting.

### Development

- Switched the platform backend library to SDL.

## 0.1.0-alpha (10/05/2022)

- Initial release.
