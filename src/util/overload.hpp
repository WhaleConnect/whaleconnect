// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file
 * @brief A struct to apply the overload pattern to @p std::visit()
*/

#pragma once

/**
 * @brief A struct to apply the overload pattern to @p std::visit().
 * @tparam ...Ts A variadic sequence of lambdas/functors to apply to the visit operation
*/
template<class... Ts>
struct Overload : Ts... {
    using Ts::operator()...;
};
