// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if __has_include(<format>)
// #include <format> exists, directly use it
#include <format> // std::format() [C++20]

namespace std2 {
    using std::format;
}
#else
// #include <format> doesn't exist, use {fmt} as a fallback
#include <fmt/core.h>

namespace std2 {
    using fmt::format;
}
#endif
