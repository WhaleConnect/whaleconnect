// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
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
    private var finished = true
    private var result = kIOReturnSuccess
    private var cond = NSCondition()
    private var timeout: DispatchWorkItem?

    // Timeout for creating baseband connections (seconds)
    private static let timeoutDuration = 10.0

    func remoteNameRequestComplete(_: IOBluetoothDevice!, status _: IOReturn) {}

    func connectionComplete(_ device: IOBluetoothDevice!, status: IOReturn) {
        // Deactivate timeout
        timeout?.cancel()

        if status == kIOReturnSuccess {
            // Perform query if connection succeeded
            let queryResult = device.performSDPQuery(self)
            if queryResult != kIOReturnSuccess {
                finishWait(newStatus: queryResult)
            }
        } else {
            finishWait(newStatus: status)
        }
    }

    func sdpQueryComplete(_: IOBluetoothDevice!, status: IOReturn) {
        finishWait(newStatus: status)
    }

    private func timeoutFn() {
        if !finished {
            finishWait(newStatus: kIOReturnTimeout)
        }
    }

    private func finishWait(newStatus: IOReturn) {
        cond.lock()
        finished = true
        result = newStatus
        cond.signal()
        cond.unlock()
    }

    func startQuery(device: IOBluetoothDevice) {
        // Since Monterey, performSDPQuery does not open a baseband connection, so this must be done before querying
        device.openConnection(self)
        cond.lock()
        finished = false

        // Activate timeout for connecting
        timeout = DispatchWorkItem(block: timeoutFn)
        DispatchQueue.main.asyncAfter(deadline: .now() + InquiryHandler.timeoutDuration, execute: timeout!)
    }

    func wait() -> IOReturn {
        while !finished {
            cond.wait()
        }

        cond.unlock()
        return result
    }
}

func getUUIDInt(data: IOBluetoothSDPDataElement) -> UInt16 {
    let uuid = data.getUUIDValue()!
    return UInt16(bigEndian: UInt16(uuid[2]) | (UInt16(uuid[3]) << 8))
}

func checkProtocolAttributes(p: IOBluetoothSDPDataElement, protoUUIDs: inout [UInt16],
                             port: inout UInt16) -> IOBluetoothSDPUUID
{
    var proto: UInt16 = 0

    for case let data as IOBluetoothSDPDataElement in p.getArrayValue() {
        switch Int(data.getTypeDescriptor()) {
            case kBluetoothSDPDataElementTypeUUID:
                proto = getUUIDInt(data: data)
                protoUUIDs.append(proto)
            case kBluetoothSDPDataElementTypeUnsignedInt:
                let val = data.getNumberValue().intValue
                let isRFCOMM = proto == kBluetoothSDPUUID16RFCOMM
                let isL2CAP = proto == kBluetoothSDPUUID16L2CAP

                // Get port - make sure size matches the protocol
                // RFCOMM channel is stored in an 8-bit integer (1 byte)
                // L2CAP channel is stored in a 16-bit integer (2 bytes)
                if (val <= 0xFF && isRFCOMM) || (val < 0xFFFF && isL2CAP) {
                    port = UInt16(data.getNumberValue().intValue)
                }
            default:
                break
        }
    }

    // For comparing and filtering protocol UUIDs
    return IOBluetoothSDPUUID(uuid16: proto)
}

public func getPairedDevices() -> [PairedDevice] {
    guard let pairedDevices = IOBluetoothDevice.pairedDevices() else {
        return []
    }

    var ret: [PairedDevice] = []
    for case let device as IOBluetoothDevice in pairedDevices {
        let address = device.addressString.replacingOccurrences(of: "-", with: ":").uppercased()

        // pairedDevices may return duplicates if a device is not removed but paired again
        // Uniqueness filter is based on address.
        if !ret.contains(where: { $0.address == address }) {
            ret.append(PairedDevice(name: device.name, address: address))
        }
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
        handler.startQuery(device: device)

        // Wait for the query to end and check if it failed asynchronously
        let res = handler.wait()
        if device.isConnected() {
            device.closeConnection()
        }

        if res != kIOReturnSuccess {
            return LookupResult(result: res, list: [])
        }
    }

    let offset: UInt32 = 256 // "language base offset" mentioned in header
    let nameKey = UInt16(kBluetoothSDPAttributeIdentifierServiceName.rawValue + offset)
    let descKey = UInt16(kBluetoothSDPAttributeIdentifierServiceDescription.rawValue + offset)
    let protosKey = UInt16(kBluetoothSDPAttributeIdentifierProtocolDescriptorList.rawValue)
    let servicesKey = UInt16(kBluetoothSDPAttributeIdentifierServiceClassIDList.rawValue)
    let profilesKey = UInt16(kBluetoothSDPAttributeIdentifierBluetoothProfileDescriptorList.rawValue)

    // Filtering UUIDs manually:
    //   - performSDPQuery(_:uuids:) does nothing since Monterey so performSDPQuery(_:) must be used which returns all
    //     services on the target.
    //   - Using cached SDP data without performing a query also returns everything.
    // SDP results are filtered on both protocol and service UUIDs.
    let filterUUID = IOBluetoothSDPUUID(bytes: uuid, length: 16)

    var ret: [SDPResult] = []
    for case let rec as IOBluetoothSDPServiceRecord in device.services {
        // Name and description (may be null if not set)
        let name = rec.getAttributeDataElement(nameKey)?.getStringValue() ?? ""
        let desc = rec.getAttributeDataElement(descKey)?.getStringValue() ?? ""

        // Protocol descriptors
        var protoUUIDs: [UInt16] = []
        var port: UInt16 = 0

        var filter = false

        rec.getAttributeDataElement(protosKey)?.getArrayValue().forEach {
            let protoUUID = checkProtocolAttributes(
                p: $0 as! IOBluetoothSDPDataElement,
                protoUUIDs: &protoUUIDs,
                port: &port
            )

            if protoUUID.isEqual(to: filterUUID) {
                filter = true
            }
        }

        // Service class UUIDs
        var serviceUUIDs: [UnsafeMutableRawPointer] = []
        let getUUID = { (data: IOBluetoothSDPDataElement) in
            // 128-bit UUID = 16 bytes of memory
            let uuid128 = UnsafeMutablePointer<UInt8>.allocate(capacity: 16)

            if let currentUUID = data.getUUIDValue().getWithLength(16) {
                currentUUID.getBytes(uuid128, length: 16)
                serviceUUIDs.append(uuid128)
                if currentUUID.isEqual(to: filterUUID) {
                    filter = true
                }
            }
        }

        // Services may be reported as a list of elements, or a single element
        if let services = rec.getAttributeDataElement(servicesKey) {
            switch Int(services.getTypeDescriptor()) {
                case kBluetoothSDPDataElementTypeUUID:
                    getUUID(services)
                case kBluetoothSDPDataElementTypeDataElementSequence:
                    services.getArrayValue()!.map { $0 as! IOBluetoothSDPDataElement }.forEach(getUUID)
                default:
                    break
            }
        }

        // Skip results that did not pass the filter
        if !filter {
            continue
        }

        // Profile descriptors
        var profileDescs: [ProfileDesc] = []
        rec.getAttributeDataElement(profilesKey)?.getArrayValue().forEach {
            // Data is stored in an array: [0] = UUID, [1] = version
            let profile = $0 as! IOBluetoothSDPDataElement
            let profileData = profile.getArrayValue() as! [IOBluetoothSDPDataElement]
            let version = UInt16(truncating: profileData[1].getNumberValue())
            let uuid = getUUIDInt(data: profileData[0])

            // Return raw version number (will be processed in C++)
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
