// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief Main window handling functions
*/

#pragma once

#include <SDL.h>
#include <SDL_opengl.h>

namespace App {
    /**
    * @brief Information returned from initializing SDL.
    */
    struct SDLData {
        SDL_Window *window; /**< The main application window */
        SDL_GLContext glContext; /**< The OpenGL context */
    };

    /**
     * @brief Sets up backends/context, configures Dear ImGui, and creates a main application window.
     *
     * @return The window and context created
    */
    SDLData init();

    /**
     * @brief Checks if the main window should be closed and creates a new frame at the start of every loop iteration.
     *
     * @return If the main loop should continue (window not yet closed)
    */
    bool newFrame();

    /**
     * @brief Handles the rendering of the window at the end of every loop iteration.
     *
     * @param sdlData The data obtained from initialization
    */
    void render(const SDLData& sdlData);

    /**
     * @brief Cleans up all backends and destroys the main window.
     *
     * @param sdlData The data obtained from initialization
    */
    void cleanup(const SDLData& sdlData);
}
