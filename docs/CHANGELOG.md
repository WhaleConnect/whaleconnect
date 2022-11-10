# Network Socket Terminal Changelog

This document tracks the changes between Network Socket Terminal versions. Dates are written in the MM/DD/YYYY format.

## 0.2.0-alpha (Unreleased)

### Additions

- Added support for macOS. [IN PROGRESS]
- Added a "Copy line" right-click option in connection windows.
- The "Show timestamps" feature is now available in connection windows on all platforms.
  - Timestamps are now displayed with three digits of subsecond precision instead of seven as in the previous release.

### Improvements

- Redesigned the loading spinner shown during an SDP inquiry.

### Bug Fixes

- Fixed a crash when the app is closed without successful initialization of the thread pool.
- Fixed the receive buffer to correctly handle the ends of strings.
- Fixed the textbox auto-focus in connection windows when the Enter key is pressed.

### Removals

- Removed the "send textbox height" setting.

### Development

- Switched the platform backend library to SDL.

## 0.1.0-alpha (10/05/2022)

- Initial release.
