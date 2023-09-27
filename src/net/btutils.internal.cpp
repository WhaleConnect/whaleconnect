// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#include <cstdint>

module net.btutils.internal;
import net.btutils;

void BTUtils::Internal::extractVersionNums(uint16_t version, BTUtils::ProfileDesc& desc) {
    // Bit operations to extract the two octets
    // The major and minor version numbers are stored in the high-order 8 bits and low-order 8 bits of the 16-bit
    // version number, respectively. This is the same for both Windows and Linux.
    desc.versionMajor = version >> 8;
    desc.versionMinor = version & 0xFF;
}
