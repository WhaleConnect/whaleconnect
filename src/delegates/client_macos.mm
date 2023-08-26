// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <IOKit/IOReturn.h>

#include "client.hpp"

#include "net/bthandle.h"
#include "net/enums.hpp"
#include "os/async_macos.hpp"
#include "os/error.hpp"

template <>
Task<> Delegates::Client<SocketTag::BT>::connect() {
    // The channel will not be fully opened until a delegate is registered
    // https://developer.apple.com/documentation/iobluetooth/iobluetoothdevice/1430889-openl2capchannelasync
    // https://developer.apple.com/documentation/iobluetooth/iobluetoothdevice/1435022-openrfcommchannelasync
    // This makes delegate registration similar to a connect() socket call.
    [_handle registerAsDelegate];

    NSUInteger hash = [_handle channelHash];
    co_await Async::run(std::bind_front(Async::submitIOBluetooth, hash, Async::IOType::Send),
                        System::ErrorType::IOReturn);
}
#endif
