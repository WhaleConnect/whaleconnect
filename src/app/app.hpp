// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief Main window handling functions
 */

#pragma once

namespace App {
    /**
     * @brief Sets up backends/context, configures Dear ImGui, and creates a main application window.
     *
     * @return If initialization was successful
     */
    bool init();

    /**
     * @brief Checks if the main window should be closed and creates a new frame at the start of every loop iteration.
     *
     * @return If the main loop should continue (window not yet closed)
     */
    bool newFrame();

    /**
     * @brief Handles the rendering of the window at the end of every loop iteration.
     */
    void render();

    /**
     * @brief Cleans up all backends and destroys the main window.
     */
    void cleanup();
}
