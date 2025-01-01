// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <bit>
#include <cstdint>

namespace UUIDs {
    // 128-bit UUID represented in a platform-independent way.
    using UUID128 = std::array<std::uint8_t, 16>;

    // Swaps endianness to/from UUID byte order (big endian).
    template <std::integral T>
    constexpr T byteSwap(T from) {
        if constexpr (std::endian::native == std::endian::big) return from;

        static_assert(std::endian::native == std::endian::little, "Unsupported/unknown endianness");
        return std::byteswap(from);
    }

    UUID128 fromSegments(std::uint32_t d1, std::uint16_t d2, std::uint16_t d3, std::uint64_t d4);

    // Constructs a 128-bit Bluetooth UUID given the short (16- or 32-bit) UUID.
    inline UUIDs::UUID128 createFromBase(std::uint32_t uuidShort) {
        // To turn a 16-bit UUID into a 128-bit UUID:
        //   | The 16-bit Attribute UUID replaces the x's in the following:
        //   | 0000xxxx - 0000 - 1000 - 8000 - 00805F9B34FB
        // https://stackoverflow.com/a/36212021
        // (The same applies with a 32-bit UUID)
        return UUIDs::fromSegments(uuidShort, 0x0000, 0x1000, 0x800000805F9B34FB);
    }
}
