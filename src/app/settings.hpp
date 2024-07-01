// Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include <imgui.h>

#include "utils/uuids.hpp"

namespace Settings {
    // Variables that can be configured through settings

    namespace Font {
        inline std::string file;
        inline std::vector<ImWchar> ranges;
        inline std::uint8_t size;
    }

    namespace GUI {
        inline bool roundedCorners;
        inline bool windowTransparency;
        inline bool systemMenu;
    }

    namespace OS {
        inline std::uint8_t numThreads;
        inline std::uint8_t queueEntries;
        inline std::vector<std::pair<std::string, UUIDs::UUID128>> bluetoothUUIDs;
    }

    // Loads the application settings.
    void load();

    // Saves the application settings.
    void save();

    // Renders the settings UI.
    void drawSettingsWindow(bool& open);
}
