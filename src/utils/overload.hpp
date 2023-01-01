// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// A struct to apply the overload pattern to std::visit.
template <class... Ts>
struct Overload : Ts... {
    using Ts::operator()...;
};

#ifdef __clang__
// Clang does not yet support CTAD for aggregates (P1816R0)
template <class... Ts>
Overload(Ts...) -> Overload<Ts...>;
#endif
