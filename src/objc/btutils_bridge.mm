// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_MACOS
#include "btutils_bridge.hpp"

#include <Foundation/Foundation.h>
#include <IOBluetooth/IOBluetooth.h>

#include "objc/cpp_objc_bridge.hpp"

// Interface to handle events from SDP queries.
@interface InquiryHandler : NSObject <IOBluetoothDeviceAsyncCallbacks>
@property bool finished;
@property IOReturn result;
@property (strong) NSCondition* cond;

// Waits synchronously for the query to complete.
- (IOReturn)wait;
@end

@implementation InquiryHandler

- (void)remoteNameRequestComplete:(IOBluetoothDevice*)device status:(IOReturn)status {
}

- (void)connectionComplete:(IOBluetoothDevice*)device status:(IOReturn)status {
}

- (void)sdpQueryComplete:(IOBluetoothDevice*)device status:(IOReturn)status {
    // Use a condition variable to control waiting and resuming on the main thread
    [_cond lock];
    _finished = true;
    _result = status;
    [_cond signal];
    [_cond unlock];
}

- (IOReturn)wait {
    [_cond lock];
    _finished = false;

    while (!_finished) [_cond wait];

    [_cond unlock];
    return _result;
}

@end

// Gets a uint16_t representation of a UUID object.
uint16_t getUUIDInt(IOBluetoothSDPDataElement* data) {
    uint16_t ret;
    [[data getUUIDValue] getBytes:&ret length:sizeof(ret)];

    ret = ntohs(ret);
    return ret;
}

void checkProtocolAttributes(IOBluetoothSDPDataElement* p, BTUtilsBridge::ObjCSDPResult& result) {
    uint16_t proto = 0;
    for (IOBluetoothSDPDataElement* data in [p getArrayValue]) {
        switch ([data getTypeDescriptor]) {
            case kBluetoothSDPDataElementTypeUUID:
                // Keep track of protocol UUIDs
                proto = getUUIDInt(data);
                result.protoUUIDs.push_back(proto);
                break;
            case kBluetoothSDPDataElementTypeUnsignedInt:
                uint32_t size = [data getSize];
                bool isRFCOMM = proto == kBluetoothSDPUUID16RFCOMM;
                bool isL2CAP = proto == kBluetoothSDPUUID16L2CAP;

                // Get port - make sure size matches the protocol
                // RFCOMM channel is stored in an 8-bit integer (1 byte)
                // L2CAP channel is stored in a 16-bit integer (2 bytes)
                if (((size == 1) && isRFCOMM) || ((size == 2) && isL2CAP))
                    result.port = [[data getNumberValue] integerValue];
        }
    }
}

std::vector<BTUtilsBridge::ObjCDevice> BTUtilsBridge::getPaired() {
    NSArray* devices = [IOBluetoothDevice pairedDevices];

    std::vector<BTUtilsBridge::ObjCDevice> ret;
    for (IOBluetoothDevice* i in devices) {
        // Get name and address (dashes in address string are replaced with colons)
        NSString* name = [i name];
        NSString* addr = [[i addressString] stringByReplacingOccurrencesOfString:@"-" withString:@":"];
        ret.emplace_back([name UTF8String], [addr UTF8String]);
    }

    return ret;
}

std::vector<BTUtilsBridge::ObjCSDPResult> BTUtilsBridge::sdpLookup(std::string_view addr, uint8_t* uuid,
                                                                   bool flushCache) {
    auto device = [IOBluetoothDevice deviceWithAddressString:@(addr.data())];

    // Issue new query if requested
    if (flushCache) {
        constexpr const char* fnName = "performSDPQuery";

        auto handler = [InquiryHandler new];
        NSArray* uuids = @[ [IOBluetoothSDPUUID uuidWithBytes:uuid length:16] ];
        IOReturn res = [device performSDPQuery:handler uuids:uuids];

        // Check result of starting the query
        if (res != kIOReturnSuccess) CppObjCBridge::throwSystemError(res, fnName);

        // Wait for the query to end and check if it failed asynchronously
        auto waitRes = [handler wait];
        if (waitRes != kIOReturnSuccess) CppObjCBridge::throwSystemError(waitRes, fnName);
    }

    // SDP attribute IDs (not defined in IOBluetooth headers)
    constexpr auto SDP_ATTR_NAME = 256;    // Name
    constexpr auto SDP_ATTR_DESC = 257;    // Description
    constexpr auto SDP_ATTR_PROTOCOLS = 4; // Protocol descriptor list
    constexpr auto SDP_ATTR_SERVICES = 1;  // Service class UUID list
    constexpr auto SDP_ATTR_PROFILES = 9;  // Profile descriptor list

    std::vector<ObjCSDPResult> ret;
    for (IOBluetoothSDPServiceRecord* rec in [device services]) {
        ObjCSDPResult result;

        // Name and description (may be null if not set)
        NSString* namePtr = [[rec getAttributeDataElement:SDP_ATTR_NAME] getStringValue];
        NSString* descPtr = [[rec getAttributeDataElement:SDP_ATTR_DESC] getStringValue];

        result.name = namePtr ? [namePtr UTF8String] : "";
        result.desc = descPtr ? [descPtr UTF8String] : "";

        // Protocol descriptors
        for (IOBluetoothSDPDataElement* p in [[rec getAttributeDataElement:SDP_ATTR_PROTOCOLS] getArrayValue])
            checkProtocolAttributes(p, result);

        // Service class UUIDs
        for (IOBluetoothSDPDataElement* data in [[rec getAttributeDataElement:SDP_ATTR_SERVICES] getArrayValue]) {
            // 128-bit UUID (16 bytes)
            ObjCUUID128 uuid128;
            [[[data getUUIDValue] getUUIDWithLength:16] getBytes:uuid128.data() length:16];
            result.serviceUUIDs.push_back(uuid128);
        }

        // Profile descriptors
        for (IOBluetoothSDPDataElement* profile in [[rec getAttributeDataElement:SDP_ATTR_PROFILES] getArrayValue]) {
            // Data is stored in an array: [0] = UUID, [1] = version
            NSArray* profileData = [profile getArrayValue];
            uint16_t version = [[profileData[1] getNumberValue] integerValue];

            ObjCProfileDesc pd;
            pd.uuid = getUUIDInt(profileData[0]);
            BTUtilsBridge::extractVersionNums(version, pd);
            result.profileDescs.push_back(pd);
        }

        ret.push_back(result);
    }

    return ret;
}
#endif
