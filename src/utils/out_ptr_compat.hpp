// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if __cpp_lib_out_ptr >= 202106L
// out_ptr exists, directly use it
#include <memory> // std::out_ptr() [C++23]

namespace std2 {
    using std::out_ptr;
}
#else
// out_ptr doesn't exist, use ztd.out_ptr as a fallback
#include <ztd/out_ptr.hpp>

namespace std2 {
    using ztd::out_ptr::out_ptr;
}
#endif
