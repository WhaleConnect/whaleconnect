// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <nameof.hpp>

#define CHECK(f, ...) check(f, NAMEOF(f) __VA_OPT__(,) __VA_ARGS__)
