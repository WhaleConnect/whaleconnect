// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_APPLE
#include <cstdint>

#include <IOKit/IOReturn.h>

namespace ObjC {
    IOReturn sdpLookup(const char* addr, const uint8_t* uuid, bool flushCache);
}
#endif
