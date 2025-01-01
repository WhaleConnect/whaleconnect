// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

// Provides operator() for function pointers.
template <class T, auto Fn>
struct DeleterAdapter {
    void operator()(T* p) const {
        Fn(p);
    }
};

// Type alias to manage system handles with RAII.
// T: the type of the handle to manage (pointer removed)
// Fn: the address of the function to free the handle, taking a parameter of T*
template <class T, auto Fn>
using HandlePtr = std::unique_ptr<T, DeleterAdapter<T, Fn>>;
