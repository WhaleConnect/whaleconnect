// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <string>

#include <IOKit/IOReturn.h>

// Wrappers for C++ async functions.
// The hash of the Bluetooth handle is passed to C++ code for identification.

void clearDataQueue(unsigned long id);

void newData(unsigned long id, const char* data, size_t dataLen);

void outgoingComplete(unsigned long id, IOReturn status);

// Using void* to remove dependency on Swift-generated handle type
void acceptComplete(unsigned long id, bool isL2CAP, const void* channel, const std::string& name,
    const std::string& addr, uint16_t port);

void closed(unsigned long id);
