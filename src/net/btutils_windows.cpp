// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#if OS_WINDOWS
#include "btutils_internal.hpp"

#include <bit>
#include <cstring> // std::memcpy()
#include <format>
#include <memory>

#include <WinSock2.h>
#include <bluetoothapis.h>

#include "btutils.hpp"
#include "device.hpp"

#include "os/errcheck.hpp"
#include "utils/handleptr.hpp"
#include "utils/out_ptr_compat.hpp"
#include "utils/strings.hpp"

// Converts a Windows GUID struct into a UUID128.
static BTUtils::UUID128 toUUID(GUID in) {
    BTUtils::UUID128 ret;

    // Fields in a GUID structure have a system-dependent endianness, while bytes in a UUID128 are ordered based on
    // network byte ordering. The ntohl/ntohs function are used to convert the byte ordering before copying.
    uint32_t data1 = ntohl(in.Data1);
    uint16_t data2 = ntohs(in.Data2);
    uint16_t data3 = ntohs(in.Data3);

    // Copy data into return object, the destination pointer is incremented by the sum of the previous sizes
    // Bytes stored in an array are guaranteed to be contiguous, while different fields in a struct may have padding in
    // between, so memcpy is used to populate the return object.
    std::memcpy(ret.data(), &data1, 4);
    std::memcpy(ret.data() + 4, &data2, 2);
    std::memcpy(ret.data() + 6, &data3, 2);
    std::memcpy(ret.data() + 8, &in.Data4, 8);

    return ret;
}

// Converts a UUID128 into a Windows GUID struct.
static GUID fromUUID(BTUtils::UUID128 in) {
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
static std::vector<SDP_ELEMENT_DATA> getSDPListData(SDP_ELEMENT_DATA& element) {
    std::vector<SDP_ELEMENT_DATA> ret;

    LPBYTE value = element.data.sequence.value;
    ULONG length = element.data.sequence.length;

    HBLUETOOTH_CONTAINER_ELEMENT iter = nullptr;
    while (BluetoothSdpGetContainerElementData(value, length, &iter, &element) == NO_ERROR) ret.push_back(element);

    return ret;
}

// Gets the SDP container data associated with an SDP attribute.
static std::vector<SDP_ELEMENT_DATA> getSDPListData(LPBLOB blob, USHORT attrib) {
    std::vector<SDP_ELEMENT_DATA> ret;

    // Get the list data by reading from the blob (the SDP stream)
    SDP_ELEMENT_DATA element{};
    call(FN(BluetoothSdpGetAttributeValue, blob->pBlobData, blob->cbSize, attrib, &element), checkZero, useReturnCode);
    return getSDPListData(element);
}

BTUtils::Instance::Instance() = default;

BTUtils::Instance::~Instance() = default;

DeviceList BTUtils::getPaired() {
    DeviceList deviceList;

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
    HBLUETOOTH_DEVICE_FIND foundDevice;

    try {
        foundDevice = call(FN(BluetoothFindFirstDevice, &searchCriteria, &deviceInfo), checkTrue);
    } catch (const System::SystemError& error) {
        if (error.code == ERROR_NO_MORE_ITEMS) return {}; // No paired devices

        throw;
    }

    // Loop through each found device
    do {
        // Format address byte array into MAC address
        const BYTE* addr = deviceInfo.Address.rgBytes;
        std::string mac = std::format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", addr[5], addr[4], addr[3], addr[2],
                                      addr[1], addr[0]);

        // Get the name of the device
        std::string name = Strings::fromSys(deviceInfo.szName);

        // Add to results
        deviceList.emplace_back(ConnectionType::None, name, mac, uint16_t{ 0 });
    } while (BluetoothFindNextDevice(foundDevice, &deviceInfo));

    return deviceList;
}

BTUtils::SDPResultList BTUtils::sdpLookup(std::string_view addr, UUID128 uuid, bool flushCache) {
    SDPResultList ret;

    Strings::SysStr addrWide = Strings::toSys(addr);
    GUID guid = fromUUID(uuid);

    // Set up the query set restrictions
    WSAQUERYSET wsaQuery{
        .dwSize = sizeof(WSAQUERYSET),
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
        call(FN(WSALookupServiceBegin, &wsaQuery, flags, std2::out_ptr(lookup)));
    } catch (const System::SystemError& error) {
        if (error.code == WSASERVICE_NOT_FOUND) return {}; // No services found

        throw;
    }

    // Continue the lookup
    DWORD size = 2048;
    std::vector<BYTE> resultsBuf(size);
    auto wsaResults = std::bit_cast<LPWSAQUERYSET>(resultsBuf.data());
    wsaResults->dwSize = size;
    wsaResults->dwNameSpace = NS_BTH;

    // Get various service information
    while (WSALookupServiceNext(lookup.get(), flags, &size, wsaResults) == NO_ERROR) {
        SDPResult result;

        // The name and description
        result.name = Strings::fromSys(wsaResults->lpszServiceInstanceName);
        result.desc = Strings::fromSys(wsaResults->lpszComment);

        // Protocol descriptors
        auto protoList = getSDPListData(wsaResults->lpBlob, SDP_ATTRIB_PROTOCOL_DESCRIPTOR_LIST);
        if (protoList.empty()) continue; // Contains the port, which is required for connecting

        // Iterate through the list of protocol descriptors (UUIDs + port)
        for (auto& element : protoList) {
            uint16_t proto = 0;
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
                        break;
                    default:
                        // Other types not handled
                        break;
                }
            }
        }

        // Service class UUIDs
        for (const auto& [_, specificType, data] : getSDPListData(wsaResults->lpBlob, SDP_ATTRIB_CLASS_ID_LIST)) {
            switch (specificType) {
                case SDP_ST_UUID16:
                    result.serviceUUIDs.push_back(createUUIDFromBase(data.uuid16));
                    break;
                case SDP_ST_UUID32:
                    result.serviceUUIDs.push_back(createUUIDFromBase(data.uuid32));
                    break;
                case SDP_ST_UUID128:
                    result.serviceUUIDs.push_back(toUUID(data.uuid128));
                    break;
                default:
                    break; // Other types not handled
            }
        }

        // Profile descriptors
        for (auto& element : getSDPListData(wsaResults->lpBlob, SDP_ATTRIB_PROFILE_DESCRIPTOR_LIST)) {
            // Construct a profile descriptor and populate it with the data in the nested container
            ProfileDesc pd;
            for (const auto& [_, specificType, data] : getSDPListData(element)) {
                if (specificType == SDP_ST_UUID16) pd.uuid = data.uuid16;
                else if (specificType == SDP_ST_UINT16) Internal::extractVersionNums(data.uint16, pd);
            }

            result.profileDescs.push_back(pd);
        }

        // Add to return vector
        ret.push_back(result);
    }

    return ret;
}
#endif
