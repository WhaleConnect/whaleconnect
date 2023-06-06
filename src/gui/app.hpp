// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace App {
    // Sets up backends/context, configures Dear ImGui, and creates a main application window.
    bool init();

    // Checks if the main window should be closed and creates a new frame at the start of every loop iteration.
    bool newFrame();

    // Handles the rendering of the window at the end of every loop iteration.
    void render();

    // Cleans up all backends and destroys the main window.
    void cleanup();

    // Gets the screen's scale factor.
    float getScale();
}
