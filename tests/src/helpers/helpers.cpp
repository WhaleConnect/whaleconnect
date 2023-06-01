// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "helpers.hpp"

#include <fstream>

#include <nlohmann/json.hpp>

nlohmann::json loadSettings() {
    std::ifstream f{ SETTINGS_FILE };
    return nlohmann::json::parse(f);
}
