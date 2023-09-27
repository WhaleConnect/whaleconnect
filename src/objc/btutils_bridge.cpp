// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include "btutils_bridge.hpp"

#include <bit>

import net.btutils;
import net.btutils.internal;

void BTUtilsBridge::extractVersionNums(uint16_t version, ObjCProfileDesc& pd) {
    BTUtils::Internal::extractVersionNums(version, *std::bit_cast<BTUtils::ProfileDesc*>(&pd));
}
#endif
