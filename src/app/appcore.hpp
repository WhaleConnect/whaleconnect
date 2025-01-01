// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace AppCore {
    // Re-applies Dear ImGui configuration before the next frame according to app settings.
    void configOnNextFrame();

    // Sets up backends/context, configures Dear ImGui, and creates a main application window.
    bool init();

    // Checks if the main window should be closed and creates a new frame at the start of every loop iteration.
    bool newFrame();

    // Handles the rendering of the window at the end of every loop iteration.
    void render();

    // Cleans up all backends and destroys the main window.
    void cleanup();
}
