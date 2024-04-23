// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include <imgui.h>

#include "utils/uuids.hpp"

namespace Settings {
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

    void load();

    void save();

    void drawSettingsWindow(bool& open);
}
