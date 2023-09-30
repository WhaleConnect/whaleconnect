// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>

#include <IOKit/IOReturn.h>

// Wrappers for C++ async functions.
// The hash of the channel is passed to C++ code for identification of each channel.

void clearDataQueue(unsigned long channelHash);

void newData(unsigned long channelHash, const char* data, size_t dataLen);

void outgoingComplete(unsigned long channelHash, IOReturn status);

void closed(unsigned long channelHash);