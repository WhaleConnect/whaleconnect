// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

// Type alias to manage system handles with RAII.
// T: the type of the handle to manage (pointer removed)
// Fn: the address of the function to free the handle, taking a parameter of T*
template <class T, auto Fn>
using HandlePtr = std::unique_ptr<T, decltype([](T* p) { Fn(p); })>;
