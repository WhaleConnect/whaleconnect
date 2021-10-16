// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Application configuration

#pragma once

/// <summary>
/// Namespace containing application configuration parameters.
/// </summary>
namespace Settings {
    inline unsigned char fontSize = 13; // Application font height in pixels
    inline unsigned char connectTimeout = 5; // Number of seconds to allow for connection before it aborts
    inline unsigned char sendTextboxHeight = 4; // Number of lines that can be shown in send textboxes at a time
    inline bool showFPScounter = false; // If a framerate counter is shown in the top-right corner
    inline bool roundedCorners = false; // If corners in the UI are rounded
}
