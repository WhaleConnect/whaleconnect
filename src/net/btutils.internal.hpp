// Copyright 2021-2025 Aidan Sun and the WhaleConnect contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

#include "btutils.hpp"

namespace BTUtils::Internal {
    // Gets the major and minor version numbers from a profile descriptor.
    inline void extractVersionNums(std::uint16_t version, BTUtils::ProfileDesc& desc) {
        // Bit operations to extract the two octets
        // The major and minor version numbers are stored in the high-order 8 bits and low-order 8 bits of the 16-bit
        // version number, respectively. This is the same for both Windows and Linux.
        desc.versionMajor = version >> 8;
        desc.versionMinor = version & 0xFF;
    }
}
