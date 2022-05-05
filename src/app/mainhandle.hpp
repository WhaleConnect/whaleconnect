// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief Main window handling functions
*/

#pragma once

namespace MainHandler {
    /**
     * @brief Sets up backends/context, configures Dear ImGui, and creates a main application window.
     * @return If initialization was successful
    */
    bool initApp();

    /**
     * @brief Checks if the main window is active (should not be closed).
     * @return If the window is still open (user has not clicked the X button)
    */
    bool isActive();

    /**
     * @brief Creates a new frame at the start of every main loop iteration.
    */
    void handleNewFrame();

    /**
     * @brief Handles the rendering of the window at the end of every loop iteration.
    */
    void renderWindow();

    /**
     * @brief Cleans up all backends and destroys the main window.
    */
    void cleanupApp();
}
