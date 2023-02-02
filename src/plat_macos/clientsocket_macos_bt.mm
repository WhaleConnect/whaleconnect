// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include <IOBluetooth/IOBluetooth.h>
#include <IOKit/IOReturn.h>

#include "async_macos.hpp"
#include "os/error.hpp"
#include "sockets/clientsocket.hpp"
#include "sockets/traits.hpp"

void newData(id channel, const char* data, size_t dataLen) {
    Async::bluetoothReadComplete([channel hash], data, dataLen);
}

void outgoingComplete(id channel, IOReturn status) { Async::bluetoothComplete([channel hash], status); }

@interface ChannelDelegate : NSObject <IOBluetoothL2CAPChannelDelegate, IOBluetoothRFCOMMChannelDelegate>
@end

@implementation ChannelDelegate

- (void)l2capChannelClosed:(IOBluetoothL2CAPChannel*)l2capChannel {
    newData(l2capChannel, "", 0);
}

- (void)l2capChannelData:(IOBluetoothL2CAPChannel*)l2capChannel data:(void*)dataPointer length:(size_t)dataLength {
    newData(l2capChannel, static_cast<const char*>(dataPointer), dataLength);
}

- (void)l2capChannelOpenComplete:(IOBluetoothL2CAPChannel*)l2capChannel status:(IOReturn)error {
    outgoingComplete(l2capChannel, error);
}

- (void)l2capChannelWriteComplete:(IOBluetoothL2CAPChannel*)l2capChannel refcon:(void*)refcon status:(IOReturn)error {
    outgoingComplete(l2capChannel, error);
}

- (void)rfcommChannelClosed:(IOBluetoothRFCOMMChannel*)rfcommChannel {
    newData(rfcommChannel, "", 0);
}

- (void)rfcommChannelData:(IOBluetoothRFCOMMChannel*)rfcommChannel data:(void*)dataPointer length:(size_t)dataLength {
    newData(rfcommChannel, static_cast<const char*>(dataPointer), dataLength);
}

- (void)rfcommChannelOpenComplete:(IOBluetoothRFCOMMChannel*)rfcommChannel status:(IOReturn)error {
    outgoingComplete(rfcommChannel, error);
}

- (void)rfcommChannelWriteComplete:(IOBluetoothRFCOMMChannel*)rfcommChannel
                            refcon:(void*)refcon
                            status:(IOReturn)error {
    outgoingComplete(rfcommChannel, error);
}

@end

template <>
std::unique_ptr<ClientSocket<SocketTag::BT>> createClientSocket(const Device& device) {
    auto target = [IOBluetoothDevice deviceWithAddressString:@(device.address.c_str())];
    IOReturn result = kIOReturnSuccess;
    id channel = nil;

    const char* fnName = "<opening channel>";

    using enum ConnectionType;
    switch (device.type) {
        case L2CAPStream: result = [target openL2CAPChannelAsync:&channel withPSM:device.port delegate:nil]; break;
        case RFCOMM: result = [target openRFCOMMChannelAsync:&channel withChannelID:device.port delegate:nil]; break;
        default: throw System::SystemError{ kIOReturnUnsupported, System::ErrorType::IOReturn, fnName };
    }

    if (result != kIOReturnSuccess) throw System::SystemError{ result, System::ErrorType::IOReturn, fnName };

    return std::make_unique<ClientSocket<SocketTag::BT>>(channel, device);
}

template <>
Task<> ClientSocket<SocketTag::BT>::connect() const {
    auto delegate = [ChannelDelegate new];

    IOReturn res;

    if (_device.type == ConnectionType::RFCOMM)
        res = [static_cast<IOBluetoothRFCOMMChannel*>(_get()) setDelegate:delegate];
    else res = [static_cast<IOBluetoothL2CAPChannel*>(_get()) setDelegate:delegate];

    if (res != kIOReturnSuccess) throw System::SystemError{ res, System::ErrorType::IOReturn, "setDelegate" };

    co_await Async::run([this](Async::CompletionResult& result) { Async::submitIOBluetooth([_get() hash], result); });
}
#endif
