// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

#include "components/windowlist.hpp"

namespace Menu {
    // Draws the main menu bar.
    void drawMenuBar(bool& quit, WindowList& connections, WindowList& servers);

    void setupMenuBar();

    void addWindowMenuItem(std::string_view name);

    void removeWindowMenuItem(std::string_view name);

    void addServerMenuItem(std::string_view name);

    void removeServerMenuItem(std::string_view name);
}
