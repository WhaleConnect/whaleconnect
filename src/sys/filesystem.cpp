// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#include <Windows.h>

#include "util/strings.hpp"
#endif

#include "filesystem.hpp"

fs::path System::getProgramDir() {
#ifdef _WIN32
    std::wstring path(_MAX_PATH, '\0');
    GetModuleFileName(nullptr, path.data(), static_cast<DWORD>(path.size()));
    return fs::path{ Strings::fromSys(path) }.parent_path();
#endif
}
