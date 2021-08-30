// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Main window handling functions (e.g. create/destroy, render)

#pragma once

#ifdef _MSC_VER
// Use WinMain() as an entry point on MSVC
#define MAIN_FUNC CALLBACK WinMain
#define MAIN_ARGS _In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int
#else
// Use the standard main() as an entry point on other compilers
#define MAIN_FUNC main
#define MAIN_ARGS
#endif

/// <summary>
/// Namespace containing functions to handle the app's main GUI window.
/// </summary>
namespace MainHandler {
    /// <summary>
    /// Set up backends/context, configure Dear ImGui, and create a main application window.
    /// </summary>
    /// <returns>If the initialization was successful</returns>
    bool initApp();

    /// <summary>
    /// Check if the main window is active (should not be closed).
    /// </summary>
    /// <returns>If the window is still open (user has not clicked the X button)</returns>
    bool isActive();

    /// <summary>
    /// Create a new frame at the start of every main loop iteration.
    /// </summary>
    void handleNewFrame();

    /// <summary>
    /// Handle the rendering of the window at the end of every loop iteration.
    /// </summary>
    void renderWindow();

    /// <summary>
    /// Clean up all backends and destroy the main window.
    /// </summary>
    void cleanupApp();
}
