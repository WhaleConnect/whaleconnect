// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>

#if OS_WINDOWS
#include <Windows.h>
#elif OS_APPLE
#include <libproc.h>
#include <unistd.h>

#include "errcheck.hpp"
#elif OS_LINUX
#include <limits.h>
#include <unistd.h>

#include "errcheck.hpp"
#endif

#include "filesystem.hpp"
#include "util/strings.hpp"

fs::path System::getProgramDir() {
#if OS_WINDOWS
    wchar_t path[_MAX_PATH]{};
    GetModuleFileName(nullptr, path, static_cast<DWORD>(std::ssize(path)));
#elif OS_APPLE
    char path[PROC_PIDPATHINFO_MAXSIZE]{};
    EXPECT_NONERROR(proc_pidpath, getpid(), path, std::ssize(path));
#elif OS_LINUX
    char path[PATH_MAX]{};
    EXPECT_NONERROR(readlink, "/proc/self/exe", path, std::ssize(path));
#endif

    return fs::path{ Strings::fromSys(path) }.parent_path();
}
