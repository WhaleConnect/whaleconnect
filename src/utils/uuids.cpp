// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "uuids.hpp"

#include <cstring>

UUIDs::UUID128 UUIDs::fromSegments(std::uint32_t d1, std::uint16_t d2, std::uint16_t d3, std::uint64_t d4) {
    UUIDs::UUID128 ret;

    // Input fields have a system-dependent endianness, while bytes in a UUID128 are ordered based on network byte
    // ordering
    d1 = byteSwap(d1);
    d2 = byteSwap(d2);
    d3 = byteSwap(d3);
    d4 = byteSwap(d4);

    // Copy data into return object, the destination pointer is incremented by the sum of the previous sizes
    std::memcpy(ret.data(), &d1, 4);
    std::memcpy(ret.data() + 4, &d2, 2);
    std::memcpy(ret.data() + 6, &d3, 2);
    std::memcpy(ret.data() + 8, &d4, 8);
    return ret;
}
