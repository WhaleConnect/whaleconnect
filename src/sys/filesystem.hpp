// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief Filesystem utilities not in the standard library
*/

#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace System {
    /**
     * @brief Gets the location of the executable.
     * @return The path of the directory containing the executable
    */
    fs::path getProgramDir();
}
