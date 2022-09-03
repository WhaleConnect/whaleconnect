// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief Functions to draw the tab for a new Bluetooth connection
*/

#pragma once

#include "gui/windowlist.hpp"

/**
 * @brief Renders the tab in the "New Connection" window for Bluetooth-based connections.
 * @param connections The list of @p ConnWindow objects to add to when making a new connection
 * @param sdpWindows The list of @p SDPWindow objects to add to when displaying connection options
*/
void drawBTConnectionTab(WindowList& connections, WindowList& sdpWindows);
