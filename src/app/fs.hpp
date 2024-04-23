// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace AppFS {
    fs::path getBasePath();

    fs::path getSettingsPath();
}
