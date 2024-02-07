// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module utils.uuids;
import external.std;

UUIDs::UUID128 UUIDs::fromSegments(u32 d1, u16 d2, u16 d3, u64 d4) {
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
