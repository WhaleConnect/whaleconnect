// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if OS_MACOS
#include <cstddef>
#include <cstdint>
#include <string_view>

#include <IOKit/IOReturn.h>

#include "bthandle.h"

#if __OBJC__
#include <IOBluetooth/IOBluetooth.h>
#else
class IOBluetoothObject;
#endif

namespace CppObjCBridge {
    [[noreturn]] void throwSystemError(IOReturn res, std::string_view fnName);

    uint64_t getChannelHash(IOBluetoothObject* channel);

    uint64_t getBTHandleHash(BTHandle* handle);

    namespace Async {
        // Wrappers for C++ async functions.
        // The hash of the channel is passed to C++ code for identification of each channel.

        void clearDataQueue(IOBluetoothObject* channel);

        void newData(IOBluetoothObject* channel, const char* data, size_t dataLen);

        void outgoingComplete(IOBluetoothObject* channel, IOReturn status);

        void closed(IOBluetoothObject* channel);
    }

    namespace Bluetooth {
        BTHandle* connect(std::string_view address, uint16_t port, bool isL2CAP);

        void write(BTHandle* handle, std::string_view data);

        void close(BTHandle* handle);
    }
}
#endif
