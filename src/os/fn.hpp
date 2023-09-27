// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#define FN(f, ...) FnResult(f(__VA_ARGS__), #f)
