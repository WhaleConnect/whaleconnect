// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/// <summary>
/// Set up backends/context, configure Dear ImGui, and create a main application window.
/// </summary>
/// <returns>If the initialization was successful</returns>
bool initApp();

/// <summary>
/// Check if the main window is active (should not be closed).
/// </summary>
/// <returns>If the window is still open (user has not yet clicked closed it)</returns>
bool isActive();

/// <summary>
/// Create a new frame at the start of every main loop iteration.
/// </summary>
/// <remarks>
/// This function also creates a dockspace over the main viewport so windows can be docked. (This is not part of the
/// new frame handling process, this is just for convenience.)
/// </remarks>
void handleNewFrame();

/// <summary>
/// Handle the rendering of the window at the end of every loop iteration.
/// </summary>
void renderWindow();

/// <summary>
/// Clean up all backends and destroy the main window.
/// </summary>
void cleanupApp();
