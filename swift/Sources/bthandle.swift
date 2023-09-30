// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import CppBridge
import IOBluetooth

enum Channel {
    case l2cap(IOBluetoothL2CAPChannel)
    case rfcomm(IOBluetoothRFCOMMChannel)
}

// Using an array as an optional because swift::Optional does not work in C++
// ("does not satisfy isUsableInGenericContext")
public struct BTHandleResult {
    public var result: IOReturn
    public var handle: [BTHandle]
}

func getID(obj: AnyObject) -> UInt {
    return UInt(bitPattern: ObjectIdentifier(obj))
}

class BTHandleDelegate: IOBluetoothL2CAPChannelDelegate, IOBluetoothRFCOMMChannelDelegate {
    func l2capChannelClosed(_ l2capChannel: IOBluetoothL2CAPChannel!) {
        CppBridge.closed(getID(obj: l2capChannel))
    }

    func l2capChannelData(_ l2capChannel: IOBluetoothL2CAPChannel!,
                          data dataPointer: UnsafeMutableRawPointer!,
                          length dataLength: Int)
    {
        CppBridge.newData(getID(obj: l2capChannel), dataPointer, dataLength)
    }

    func l2capChannelOpenComplete(_ l2capChannel: IOBluetoothL2CAPChannel!, status error: IOReturn) {
        CppBridge.outgoingComplete(getID(obj: l2capChannel), error)
    }

    func l2capChannelWriteComplete(_ l2capChannel: IOBluetoothL2CAPChannel!,
                                   refcon _: UnsafeMutableRawPointer!,
                                   status error: IOReturn)
    {
        CppBridge.outgoingComplete(getID(obj: l2capChannel), error)
    }

    func rfcommChannelClosed(_ rfcommChannel: IOBluetoothRFCOMMChannel!) {
        CppBridge.closed(getID(obj: rfcommChannel))
    }

    func rfcommChannelData(_ rfcommChannel: IOBluetoothRFCOMMChannel!,
                           data dataPointer: UnsafeMutableRawPointer!,
                           length dataLength: Int)
    {
        CppBridge.newData(getID(obj: rfcommChannel), dataPointer, dataLength)
    }

    func rfcommChannelOpenComplete(_ rfcommChannel: IOBluetoothRFCOMMChannel!, status error: IOReturn) {
        CppBridge.outgoingComplete(getID(obj: rfcommChannel), error)
    }

    func rfcommChannelWriteComplete(_ rfcommChannel: IOBluetoothRFCOMMChannel!,
                                    refcon _: UnsafeMutableRawPointer!,
                                    status error: IOReturn)
    {
        CppBridge.outgoingComplete(getID(obj: rfcommChannel), error)
    }
}

public class BTHandle {
    let channel: Channel
    static let delegate = BTHandleDelegate()

    init(channel: Channel) {
        self.channel = channel
    }

    public func getHash() -> UInt {
        return switch channel {
            case let .l2cap(l2capChannel): getID(obj: l2capChannel)
            case let .rfcomm(rfcommChannel): getID(obj: rfcommChannel)
        }
    }

    public func close() {
        switch channel {
            case let .l2cap(l2capChannel): l2capChannel.close()
            case let .rfcomm(rfcommChannel): rfcommChannel.close()
        }
    }

    public func write(data: String) -> IOReturn {
        let length = UInt16(data.count)
        let bytes = UnsafeMutableRawPointer(mutating: (data as NSString).utf8String)

        return switch channel {
            case let .l2cap(l2capChannel): l2capChannel.writeAsync(bytes, length: length, refcon: nil)
            case let .rfcomm(rfcommChannel): rfcommChannel.writeAsync(bytes, length: length, refcon: nil)
        }
    }
}

public func makeBTHandle(address: String, port: UInt16, isL2CAP: Bool) -> BTHandleResult {
    guard let target = IOBluetoothDevice(addressString: address) else {
        return BTHandleResult(result: kIOReturnError, handle: [])
    }

    var channel: Channel

    if isL2CAP {
        var newChannel: IOBluetoothL2CAPChannel?
        let initResult = target.openL2CAPChannelAsync(&newChannel, withPSM: port, delegate: BTHandle.delegate)

        if initResult == kIOReturnSuccess {
            channel = .l2cap(newChannel!)
            CppBridge.clearDataQueue(getID(obj: newChannel!))
            return BTHandleResult(result: kIOReturnSuccess, handle: [BTHandle(channel: channel)])
        } else {
            return BTHandleResult(result: initResult, handle: [])
        }
    } else {
        var newChannel: IOBluetoothRFCOMMChannel?
        let initResult = target.openRFCOMMChannelAsync(
            &newChannel,
            withChannelID: UInt8(port),
            delegate: BTHandle.delegate
        )

        if initResult == kIOReturnSuccess {
            channel = .rfcomm(newChannel!)
            CppBridge.clearDataQueue(getID(obj: newChannel!))
            return BTHandleResult(result: kIOReturnSuccess, handle: [BTHandle(channel: channel)])
        } else {
            return BTHandleResult(result: initResult, handle: [])
        }
    }
}
