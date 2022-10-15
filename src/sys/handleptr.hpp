// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief A typedef to manage system handles with RAII
 */

#include <memory>

/**
 * @brief A typedef to manage system handles with RAII.
 * @tparam T The type of the handle to manage (pointer removed)
 * @tparam Fn The address of the function to free the handle (takes a parameter of @p T*)
 */
template <class T, auto Fn>
using HandlePtr = std::unique_ptr<T, decltype([](T* p) { Fn(p); })>;
