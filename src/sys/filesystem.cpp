// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#include <Windows.h>

#include "util/strings.hpp"
#endif

#include "filesystem.hpp"

fs::path System::getProgramDir() {
#ifdef _WIN32
    wchar_t path[_MAX_PATH + 1]{};
    GetModuleFileName(nullptr, path, _MAX_PATH);
    return fs::path{ Strings::fromSys(path) }.parent_path();
#endif
}
