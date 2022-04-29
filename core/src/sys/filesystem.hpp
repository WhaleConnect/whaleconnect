// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Filesystem utilities not in the standard library

#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace System {
    fs::path getProgramDir();
}
