// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import CppBridge
import IOBluetooth

enum Channel {
    case l2cap(IOBluetoothL2CAPChannel)
    case rfcomm(IOBluetoothRFCOMMChannel)
    case server(IOBluetoothUserNotification?)
}

// Using an array as an optional because swift::Optional does not work in C++
// ("does not satisfy isUsableInGenericContext")
public struct BTHandleResult {
    public var result: IOReturn
    public var handle: [BTHandle]
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
    private var channel: Channel
    private let delegate = BTHandleDelegate()

    init(channel: Channel) {
        self.channel = channel
        delegate.hash = UInt(bitPattern: ObjectIdentifier(self))
        CppBridge.clearDataQueue(getHash())

        switch channel {
            case let .l2cap(l2capChannel): l2capChannel.setDelegate(delegate)
            case let .rfcomm(rfcommChannel): rfcommChannel.setDelegate(delegate)
            case .server: break
        }
    }

    private func acceptComplete(device: IOBluetoothDevice?, newChannel: Channel, port: UInt16) {
        let name = std.string(device?.name ?? "")
        let addr = std.string(device?.addressString.replacingOccurrences(of: "-", with: ":").uppercased() ?? "")
        let newHandle = BTHandle(channel: newChannel)

        withUnsafePointer(to: newHandle) {
            CppBridge.acceptComplete(getHash(), true, $0, name, addr, port)
        }
    }

    @objc private func acceptL2CAP(_: IOBluetoothUserNotification, channel newChannel: IOBluetoothL2CAPChannel) {
        acceptComplete(device: newChannel.device, newChannel: .l2cap(newChannel), port: newChannel.psm)
    }

    @objc private func acceptRFCOMM(_: IOBluetoothUserNotification, channel newChannel: IOBluetoothRFCOMMChannel) {
        acceptComplete(
            device: newChannel.getDevice(),
            newChannel: .rfcomm(newChannel),
            port: UInt16(newChannel.getID())
        )
    }

    public func getHash() -> UInt {
        return delegate.hash
    }

    public func close() {
        switch channel {
            case let .l2cap(l2capChannel): l2capChannel.close()
            case let .rfcomm(rfcommChannel): rfcommChannel.close()
            case let .server(notification): notification?.unregister()
        }
    }

    public func write(data: String) -> IOReturn {
        let length = UInt16(data.count)
        let bytes = UnsafeMutableRawPointer(mutating: (data as NSString).utf8String)

        return switch channel {
            case let .l2cap(l2capChannel): l2capChannel.writeAsync(bytes, length: length, refcon: nil)
            case let .rfcomm(rfcommChannel): rfcommChannel.writeAsync(bytes, length: length, refcon: nil)
            case .server: kIOReturnNotOpen
        }
    }

    public func startServer(isL2CAP: Bool, port: UInt16) -> Bool {
        let dir = kIOBluetoothUserNotificationChannelDirectionIncoming
        let notification = isL2CAP
            ? IOBluetoothL2CAPChannel.register(
                forChannelOpenNotifications: self,
                selector: #selector(acceptL2CAP),
                withPSM: port,
                direction: dir
            )
            : IOBluetoothRFCOMMChannel.register(
                forChannelOpenNotifications: self,
                selector: #selector(acceptRFCOMM),
                withChannelID: UInt8(port),
                direction: dir
            )
        channel = .server(notification)
        return notification != nil
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

public func makeBTServerHandle() -> BTHandle {
    return BTHandle(channel: .server(nil))
}
