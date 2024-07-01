// Copyright 2021-2024 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdexcept>

#if OS_WINDOWS
#include <Objbase.h>
#include <Shlobj.h>
#include <ztd/out_ptr.hpp>

#include "utils/handleptr.hpp"
#elif OS_MACOS || OS_LINUX
#include <climits>
#include <cstdlib>

#include <pwd.h>
#include <unistd.h>

#if OS_MACOS
#include <mach-o/dyld.h>
#endif
#endif

#include "fs.hpp"

[[noreturn]] void throwBasePathError() {
    throw std::runtime_error{ "Failed to get executable path" };
}

[[noreturn]] void throwSettingsPathError() {
    throw std::runtime_error{ "Failed to get settings path" };
}

fs::path AppFS::getBasePath() {
    // First get the path to the executable itself
#if OS_WINDOWS
    std::wstring path(MAX_PATH, 0);
    if (GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size())) == 0) throwBasePathError();
#elif OS_MACOS
    std::string path(PATH_MAX, 0);
    std::uint32_t size = path.size();
    if (_NSGetExecutablePath(path.data(), &size) == -1) throwBasePathError();

    // Return path to Resources if inside app bundle
    if (path.contains(".app/Contents")) return std::filesystem::path{ path }.parent_path().parent_path() / "Resources";
#elif OS_LINUX
    std::string path(PATH_MAX, 0);
    if (readlink("/proc/self/exe", path.data(), path.size()) == -1) throwBasePathError();
#endif

    // Return the containing directory
    return std::filesystem::path{ path }.parent_path();
}

fs::path AppFS::getSettingsPath() {
#if OS_WINDOWS
    HandlePtr<WCHAR, CoTaskMemFree> dataPathPtr;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, ztd::out_ptr::out_ptr(dataPathPtr))
        != S_OK)
        throwSettingsPathError();

    std::filesystem::path dataPath{ dataPathPtr.get() };
#elif OS_MACOS || OS_LINUX
    const char* homeDir = std::getenv("HOME");
    if (!homeDir) {
        if (passwd* pwd = getpwuid(getuid()); pwd) homeDir = pwd->pw_dir;
        else throwSettingsPathError();
    }

#if OS_MACOS
    auto dataPath = std::filesystem::path{ homeDir } / "Library" / "Application Support";
#else
    auto dataPath = std::filesystem::path{ homeDir } / ".config";
#endif
#endif

    auto path = dataPath / "WhaleConnect";
    if (!std::filesystem::exists(path)) std::filesystem::create_directories(path);
    return path;
}
