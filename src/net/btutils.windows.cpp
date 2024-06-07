// Copyright 2021-2024 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "btutils.hpp"

#include <cstring>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <WinSock2.h>
#include <bluetoothapis.h>
#include <ztd/out_ptr.hpp>

#include "btutils.internal.hpp"
#include "device.hpp"
#include "enums.hpp"
#include "os/errcheck.hpp"
#include "os/error.hpp"
#include "utils/handleptr.hpp"
#include "utils/strings.hpp"
#include "utils/uuids.hpp"

// Converts a Windows GUID struct into a UUID128.
UUIDs::UUID128 toUUID(GUID in) {
    return UUIDs::fromSegments(in.Data1, in.Data2, in.Data3,
        UUIDs::byteSwap(*reinterpret_cast<std::uint64_t*>(in.Data4)));
}

// Converts a UUID128 into a Windows GUID struct.
GUID fromUUID(UUIDs::UUID128 in) {
    GUID ret;

    // Copy data
    std::memcpy(&ret.Data1, in.data(), 4);
    std::memcpy(&ret.Data2, in.data() + 4, 2);
    std::memcpy(&ret.Data3, in.data() + 6, 2);
    std::memcpy(&ret.Data4, in.data() + 8, 8);

    // Convert from host byte ordering to network byte ordering
    ret.Data1 = htonl(ret.Data1);
    ret.Data2 = htons(ret.Data2);
    ret.Data3 = htons(ret.Data3);

    return ret;
}

// Gets the SDP container data in an SDP element.
std::vector<SDP_ELEMENT_DATA> getSDPListData(SDP_ELEMENT_DATA& element) {
    std::vector<SDP_ELEMENT_DATA> ret;

    LPBYTE value = element.data.sequence.value;
    ULONG length = element.data.sequence.length;

    HBLUETOOTH_CONTAINER_ELEMENT iter = nullptr;
    while (BluetoothSdpGetContainerElementData(value, length, &iter, &element) == 0) ret.push_back(element);

    return ret;
}

// Gets the SDP container data associated with an SDP attribute.
std::vector<SDP_ELEMENT_DATA> getSDPListData(LPBLOB blob, USHORT attrib) {
    std::vector<SDP_ELEMENT_DATA> ret;

    // Get the list data by reading from the blob (the SDP stream)
    SDP_ELEMENT_DATA element{};
    check(BluetoothSdpGetAttributeValue(blob->pBlobData, blob->cbSize, attrib, &element), checkZero, useReturnCode);
    return getSDPListData(element);
}

void checkProtocolAttributes(SDP_ELEMENT_DATA& element, BTUtils::SDPResult& result) {
    std::uint16_t proto = 0;
    for (const auto& [_, specificType, data] : getSDPListData(element)) {
        switch (specificType) {
            case SDP_ST_UUID16:
                // Keep track of protocol UUIDs
                proto = data.uuid16;
                result.protoUUIDs.push_back(proto);
                break;
            case SDP_ST_UINT8:
                // RFCOMM channel is stored in an 8-bit integer
                if (proto == RFCOMM_PROTOCOL_UUID16) result.port = data.uint8;
                break;
            case SDP_ST_UINT16:
                // L2CAP PSM is stored in a 16-bit integer
                if (proto == L2CAP_PROTOCOL_UUID16) result.port = data.uint16;
        }
    }
}

UUIDs::UUID128 getUUID(const SDP_ELEMENT_DATA& element) {
    switch (element.specificType) {
        case SDP_ST_UUID16:
            return UUIDs::createFromBase(element.data.uuid16);
        case SDP_ST_UUID32:
            return UUIDs::createFromBase(element.data.uuid32);
        case SDP_ST_UUID128:
            return toUUID(element.data.uuid128);
        default:
            std::unreachable();
    }
}

BTUtils::ProfileDesc checkProfileDescriptors(SDP_ELEMENT_DATA& element) {
    // Construct a profile descriptor and populate it with the data in the nested container
    BTUtils::ProfileDesc pd;
    for (const auto& [_, specificType, data] : getSDPListData(element)) {
        if (specificType == SDP_ST_UUID16) pd.uuid = data.uuid16;
        else if (specificType == SDP_ST_UINT16) BTUtils::Internal::extractVersionNums(data.uint16, pd);
    }

    return pd;
}

BTUtils::Instance::Instance() = default;

BTUtils::Instance::~Instance() = default;

std::vector<Device> BTUtils::getPaired() try {
    std::vector<Device> deviceList;

    // Bluetooth search criteria - only return remembered (paired) devices, and don't start a new inquiry search
    BLUETOOTH_DEVICE_SEARCH_PARAMS searchCriteria{
        .dwSize = sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS),
        .fReturnAuthenticated = false,
        .fReturnRemembered = true,
        .fReturnUnknown = false,
        .fReturnConnected = false,
        .fIssueInquiry = false,
        .cTimeoutMultiplier = 0,
    };

    // Find the first device
    BLUETOOTH_DEVICE_INFO_STRUCT deviceInfo{ .dwSize = sizeof(BLUETOOTH_DEVICE_INFO_STRUCT) };
    HandlePtr<void, BluetoothFindDeviceClose> foundDevice{ check(BluetoothFindFirstDevice(&searchCriteria, &deviceInfo),
        checkTrue) };

    // Loop through each found device
    do {
        // Format address byte array into MAC address
        const BYTE* addr = deviceInfo.Address.rgBytes;
        std::string mac = std::format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", addr[5], addr[4], addr[3], addr[2],
            addr[1], addr[0]);

        // Get the name of the device
        std::string name = Strings::fromSys(deviceInfo.szName);

        // Add to results
        deviceList.emplace_back(ConnectionType::None, name, mac, std::uint16_t{ 0 });
    } while (BluetoothFindNextDevice(foundDevice.get(), &deviceInfo));

    return deviceList;
} catch (const System::SystemError& error) {
    if (error.code == ERROR_NO_MORE_ITEMS) return {}; // No paired devices

    throw;
}

std::vector<BTUtils::SDPResult> BTUtils::sdpLookup(std::string_view addr, UUIDs::UUID128 uuid, bool flushCache) {
    std::vector<BTUtils::SDPResult> ret;

    Strings::SysStr addrWide = Strings::toSys(addr);
    GUID guid = fromUUID(uuid);

    // Set up the query set restrictions
    WSAQUERYSETW wsaQuery{
        .dwSize = sizeof(WSAQUERYSETW),
        .lpServiceClassId = &guid,
        .dwNameSpace = NS_BTH,
        .lpszContext = addrWide.data(),
        .dwNumberOfCsAddrs = 0,
    };

    // Lookup service flags
    DWORD flags = LUP_RETURN_NAME | LUP_RETURN_TYPE | LUP_RETURN_ADDR | LUP_RETURN_BLOB | LUP_RETURN_COMMENT;
    if (flushCache) flags |= LUP_FLUSHCACHE;

    // Start the lookup
    HandlePtr<void, WSALookupServiceEnd> lookup;

    try {
        check(WSALookupServiceBeginW(&wsaQuery, flags, ztd::out_ptr::out_ptr(lookup)));
    } catch (const System::SystemError& error) {
        if (error.code == WSASERVICE_NOT_FOUND) return {}; // No services found

        throw;
    }

    // Continue the lookup
    DWORD size = 2048;
    std::vector<BYTE> resultsBuf(size);
    auto wsaResults = reinterpret_cast<LPWSAQUERYSETW>(resultsBuf.data());
    wsaResults->dwSize = size;
    wsaResults->dwNameSpace = NS_BTH;

    // Get various service information
    while (WSALookupServiceNextW(lookup.get(), flags, &size, wsaResults) == 0) {
        SDPResult result;

        // The name and description
        result.name = Strings::fromSys(wsaResults->lpszServiceInstanceName);
        result.desc = Strings::fromSys(wsaResults->lpszComment);

        // Protocol descriptors
        auto protoList = getSDPListData(wsaResults->lpBlob, SDP_ATTRIB_PROTOCOL_DESCRIPTOR_LIST);
        if (protoList.empty()) continue; // Contains the port, which is required for connecting

        // Iterate through the list of protocol descriptors (UUIDs + port)
        for (auto& element : protoList) checkProtocolAttributes(element, result);

        // Service class UUIDs
        for (const auto& element : getSDPListData(wsaResults->lpBlob, SDP_ATTRIB_CLASS_ID_LIST))
            result.serviceUUIDs.push_back(getUUID(element));

        // Profile descriptors
        for (auto& element : getSDPListData(wsaResults->lpBlob, SDP_ATTRIB_PROFILE_DESCRIPTOR_LIST))
            result.profileDescs.push_back(checkProfileDescriptors(element));

        // Add to return vector
        ret.push_back(result);
    }

    return ret;
}
