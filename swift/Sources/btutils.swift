// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import CppBridge
import IOBluetooth

public struct PairedDevice {
    public var name: String
    public var address: String
}

public struct ProfileDesc {
    public var uuid: UInt16
    public var version: UInt16
}

public struct SDPResult {
    public var protoUUIDs: [UInt16]
    public var serviceUUIDs: [UnsafeMutableRawPointer]
    public var profileDescs: [ProfileDesc]
    public var port: UInt16
    public var name: String
    public var desc: String
}

public struct LookupResult {
    public var result: IOReturn
    public var list: [SDPResult]
}

class InquiryHandler: IOBluetoothDeviceAsyncCallbacks {
    var finished = false
    var result = kIOReturnSuccess
    var cond = NSCondition()

    func remoteNameRequestComplete(_: IOBluetoothDevice!, status _: IOReturn) {}

    func connectionComplete(_: IOBluetoothDevice!, status _: IOReturn) {}

    func sdpQueryComplete(_: IOBluetoothDevice!, status: IOReturn) {
        cond.lock()
        finished = true
        result = status
        cond.signal()
        cond.unlock()
    }

    func wait() -> IOReturn {
        cond.lock()
        finished = false

        while !finished {
            cond.wait()
        }

        cond.unlock()
        return result
    }
}

func getUUIDInt(data: IOBluetoothSDPDataElement) -> UInt16 {
    var ret: UInt16 = 0
    data.getUUIDValue().getBytes(&ret, length: MemoryLayout.size(ofValue: ret))
    return UInt16(bigEndian: ret)
}

func checkProtocolAttributes(p: IOBluetoothSDPDataElement, protoUUIDs: inout [UInt16], port: inout UInt16) {
    var proto: UInt16 = 0

    for case let data as IOBluetoothSDPDataElement in p.getArrayValue() {
        switch Int(data.getTypeDescriptor()) {
            case kBluetoothSDPDataElementTypeUUID:
                proto = getUUIDInt(data: data)
                protoUUIDs.append(proto)
            case kBluetoothSDPDataElementTypeUnsignedInt:
                let size = data.getSize()
                let isRFCOMM = proto == kBluetoothSDPUUID16RFCOMM
                let isL2CAP = proto == kBluetoothSDPUUID16L2CAP

                // Get port - make sure size matches the protocol
                // RFCOMM channel is stored in an 8-bit integer (1 byte)
                // L2CAP channel is stored in a 16-bit integer (2 bytes)
                if (size == 1 && isRFCOMM) || (size == 2 && isL2CAP) {
                    port = UInt16(data.getNumberValue().intValue)
                }
            default:
                break
        }
    }
}

public func getPairedDevices() -> [PairedDevice] {
    let pairedDevices = IOBluetoothDevice.pairedDevices()
    if pairedDevices == nil {
        return []
    }

    var ret: [PairedDevice] = []
    for case let device as IOBluetoothDevice in pairedDevices! {
        ret.append(PairedDevice(
            name: device.name,
            address: device.addressString.replacingOccurrences(of: "-", with: ":")
        ))
    }

    return ret
}

public func sdpLookup(addr: String, uuid: UnsafeRawPointer, flushCache: Bool) -> LookupResult {
    guard let device = IOBluetoothDevice(addressString: addr) else {
        return LookupResult(result: kIOReturnError, list: [])
    }

    // Issue new query if requested
    if flushCache {
        let handler = InquiryHandler()
        let uuids = [IOBluetoothSDPUUID(bytes: uuid, length: 16)]
        let res = device.performSDPQuery(handler, uuids: uuids)

        // Check result of starting the query
        if res != kIOReturnSuccess {
            return LookupResult(result: res, list: [])
        }

        // Wait for the query to end and check if it failed asynchronously
        let waitRes = handler.wait()
        if waitRes != kIOReturnSuccess {
            return LookupResult(result: waitRes, list: [])
        }
    }

    // SDP attribute IDs (not defined in IOBluetooth headers)
    let SDP_ATTR_NAME: UInt16 = 256 // Name
    let SDP_ATTR_DESC: UInt16 = 257 // Description
    let SDP_ATTR_PROTOCOLS: UInt16 = 4 // Protocol descriptor list
    let SDP_ATTR_SERVICES: UInt16 = 1 // Service class UUID list
    let SDP_ATTR_PROFILES: UInt16 = 9 // Profile descriptor list

    var ret: [SDPResult] = []
    for case let rec as IOBluetoothSDPServiceRecord in device.services {
        // Name and description (may be null if not set)
        let name = rec.getAttributeDataElement(SDP_ATTR_NAME)?.getStringValue() ?? ""
        let desc = rec.getAttributeDataElement(SDP_ATTR_DESC)?.getStringValue() ?? ""

        // Protocol descriptors
        var protoUUIDs: [UInt16] = []
        var port: UInt16 = 0

        rec.getAttributeDataElement(SDP_ATTR_PROTOCOLS).getArrayValue()?.forEach {
            checkProtocolAttributes(p: $0 as! IOBluetoothSDPDataElement, protoUUIDs: &protoUUIDs, port: &port)
        }

        // Service class UUIDs
        var serviceUUIDs: [UnsafeMutableRawPointer] = []
        rec.getAttributeDataElement(SDP_ATTR_SERVICES).getArrayValue()?.forEach {
            let data = $0 as! IOBluetoothSDPDataElement
            let uuid128 = UnsafeMutablePointer<UInt8>.allocate(capacity: 16)

            data.getUUIDValue().getWithLength(16).getBytes(uuid128, length: 16)
            serviceUUIDs.append(uuid128)
        }

        // Profile descriptors
        var profileDescs: [ProfileDesc] = []
        rec.getAttributeDataElement(SDP_ATTR_PROFILES).getArrayValue()?.forEach {
            // Data is stored in an array: [0] = UUID, [1] = version
            let profile = $0 as! IOBluetoothSDPDataElement
            let profileData = profile.getArrayValue() as! [IOBluetoothSDPDataElement]
            let version = UInt16(truncating: profileData[1].getNumberValue())

            let uuid = getUUIDInt(data: profileData[0])
            profileDescs.append(ProfileDesc(uuid: uuid, version: version))
        }

        ret.append(SDPResult(
            protoUUIDs: protoUUIDs,
            serviceUUIDs: serviceUUIDs,
            profileDescs: profileDescs,
            port: port,
            name: name,
            desc: desc
        ))
    }

    return LookupResult(result: kIOReturnSuccess, list: ret)
}
