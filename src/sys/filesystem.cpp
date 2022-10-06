// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include <Windows.h>
#else
#include <limits.h>
#include <unistd.h>

#include "errcheck.hpp"
#endif

#include "filesystem.hpp"
#include "util/strings.hpp"

fs::path System::getProgramDir() {
#if OS_WINDOWS
    std::wstring path(_MAX_PATH, '\0');
    GetModuleFileName(nullptr, path.data(), static_cast<DWORD>(path.size()));
#else
    std::string path(PATH_MAX, '\0');
    EXPECT_NONERROR(readlink, "/proc/self/exe", path.data(), path.size());
#endif

    return fs::path{ Strings::fromSys(path) }.parent_path();
}
