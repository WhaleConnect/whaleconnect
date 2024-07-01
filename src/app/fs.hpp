// Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace AppFS {
    // Gets the directory of the executable. In a macOS app bundle, returns the path to the Contents directory.
    fs::path getBasePath();

    // Gets the path to the settings directory.
    fs::path getSettingsPath();
}
