// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <fstream>

#include <nlohmann/json.hpp>

module helpers.helpers;

nlohmann::json loadSettings() {
    std::ifstream f{ SETTINGS_FILE };
    return nlohmann::json::parse(f);
}
