// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import CppBridge
import IOBluetooth

enum Channel {
    case l2cap(IOBluetoothL2CAPChannel)
    case rfcomm(IOBluetoothRFCOMMChannel)
    case none
}

// Using an array as an optional because swift::Optional does not work in C++
// ("does not satisfy isUsableInGenericContext")
public struct BTHandleResult {
    public var result: IOReturn
    public var handle: [BTHandle]
}

func unpackDevice(device: IOBluetoothDevice?) -> (std.string, std.string) {
    let name = std.string(device?.name ?? "")
    let addr = std.string(device?.addressString.replacingOccurrences(of: "-", with: ":") ?? "")
    return (name, addr)
}

class BTHandleDelegate: IOBluetoothL2CAPChannelDelegate, IOBluetoothRFCOMMChannelDelegate {
    var hash: UInt = 0

    func l2capChannelClosed(_: IOBluetoothL2CAPChannel!) {
        CppBridge.closed(hash)
    }

    func l2capChannelData(_: IOBluetoothL2CAPChannel!,
                          data dataPointer: UnsafeMutableRawPointer!,
                          length dataLength: Int)
    {
        CppBridge.newData(hash, dataPointer, dataLength)
    }

    func l2capChannelOpenComplete(_: IOBluetoothL2CAPChannel!, status error: IOReturn) {
        CppBridge.outgoingComplete(hash, error)
    }

    func l2capChannelWriteComplete(_: IOBluetoothL2CAPChannel!,
                                   refcon _: UnsafeMutableRawPointer!,
                                   status error: IOReturn)
    {
        CppBridge.outgoingComplete(hash, error)
    }

    func rfcommChannelClosed(_: IOBluetoothRFCOMMChannel!) {
        CppBridge.closed(hash)
    }

    func rfcommChannelData(_: IOBluetoothRFCOMMChannel!,
                           data dataPointer: UnsafeMutableRawPointer!,
                           length dataLength: Int)
    {
        CppBridge.newData(hash, dataPointer, dataLength)
    }

    func rfcommChannelOpenComplete(_: IOBluetoothRFCOMMChannel!, status error: IOReturn) {
        CppBridge.outgoingComplete(hash, error)
    }

    func rfcommChannelWriteComplete(_: IOBluetoothRFCOMMChannel!,
                                    refcon _: UnsafeMutableRawPointer!,
                                    status error: IOReturn)
    {
        CppBridge.outgoingComplete(hash, error)
    }
}

public class BTHandle {
    let channel: Channel
    let delegate = BTHandleDelegate()

    var notification: IOBluetoothUserNotification?
    var clients: [BTHandle] = []

    init(channel: Channel) {
        self.channel = channel
        delegate.hash = UInt(bitPattern: ObjectIdentifier(self))
        CppBridge.clearDataQueue(getHash())

        switch channel {
            case let .l2cap(l2capChannel): l2capChannel.setDelegate(delegate)
            case let .rfcomm(rfcommChannel): rfcommChannel.setDelegate(delegate)
            case .none: break
        }
    }

    @objc func acceptL2CAP(_: IOBluetoothUserNotification, channel newChannel: IOBluetoothL2CAPChannel) {
        clients.append(BTHandle(channel: .l2cap(newChannel)))
        let (name, addr) = unpackDevice(device: newChannel.device)
        CppBridge.acceptComplete(getHash(), true, name, addr, newChannel.psm)
    }

    @objc func acceptRFCOMM(_: IOBluetoothUserNotification, channel newChannel: IOBluetoothRFCOMMChannel) {
        clients.append(BTHandle(channel: .rfcomm(newChannel)))
        let (name, addr) = unpackDevice(device: newChannel.getDevice())
        CppBridge.acceptComplete(getHash(), false, name, addr, UInt16(newChannel.getID()))
    }

    public func getHash() -> UInt {
        return delegate.hash
    }

    public func close() {
        notification?.unregister()
        switch channel {
            case let .l2cap(l2capChannel): l2capChannel.close()
            case let .rfcomm(rfcommChannel): rfcommChannel.close()
            case .none: break
        }

        for client in clients {
            client.close()
        }
    }

    public func write(data: String) -> IOReturn {
        let length = UInt16(data.count)
        let bytes = UnsafeMutableRawPointer(mutating: (data as NSString).utf8String)

        return switch channel {
            case let .l2cap(l2capChannel): l2capChannel.writeAsync(bytes, length: length, refcon: nil)
            case let .rfcomm(rfcommChannel): rfcommChannel.writeAsync(bytes, length: length, refcon: nil)
            case .none: kIOReturnNotOpen
        }
    }

    public func startServer(port: UInt16) {
        let dir = kIOBluetoothUserNotificationChannelDirectionIncoming

        notification = switch channel {
            case .l2cap:
                IOBluetoothL2CAPChannel.register(
                    forChannelOpenNotifications: self,
                    selector: #selector(acceptL2CAP),
                    withPSM: port,
                    direction: dir
                )
            case .rfcomm:
                IOBluetoothRFCOMMChannel.register(
                    forChannelOpenNotifications: self,
                    selector: #selector(acceptRFCOMM),
                    withChannelID: UInt8(port),
                    direction: dir
                )
            case .none: nil
        }
    }

    public func getLastClient() -> [BTHandle] {
        if let last = clients.last {
            return [last]
        } else {
            return []
        }
    }
}

public func makeBTHandle(address: String, port: UInt16, isL2CAP: Bool) -> BTHandleResult {
    guard let target = IOBluetoothDevice(addressString: address) else {
        return BTHandleResult(result: kIOReturnError, handle: [])
    }

    if isL2CAP {
        var newChannel: IOBluetoothL2CAPChannel?
        let initResult = target.openL2CAPChannelAsync(&newChannel, withPSM: port, delegate: nil)

        if initResult == kIOReturnSuccess {
            return BTHandleResult(result: kIOReturnSuccess, handle: [BTHandle(channel: .l2cap(newChannel!))])
        } else {
            return BTHandleResult(result: initResult, handle: [])
        }
    } else {
        var newChannel: IOBluetoothRFCOMMChannel?
        let initResult = target.openRFCOMMChannelAsync(
            &newChannel,
            withChannelID: UInt8(port),
            delegate: nil
        )

        if initResult == kIOReturnSuccess {
            return BTHandleResult(result: kIOReturnSuccess, handle: [BTHandle(channel: .rfcomm(newChannel!))])
        } else {
            return BTHandleResult(result: initResult, handle: [])
        }
    }
}
