// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <bit>

#include "btutils.hpp"
#include "sockets.hpp"

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2bth.h>
#include <bluetoothapis.h>

#include "compat/format.hpp"
#include "util/strings.hpp"
#else
#include <cstring> // std::memcpy()

#include <dbus/dbus.h>

#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

static DBusConnection* conn = nullptr;
#endif

#include "compat/outptr.hpp"
#include "sys/errcheck.hpp"
#include "sys/handleptr.hpp"

void BTUtils::init() {
    Sockets::init();

    // Connect to the system D-Bus
    conn = EXPECT_TRUE(dbus_bus_get, DBUS_BUS_SYSTEM, nullptr);

#ifndef _WIN32
    // The process will exit when a connection with dbus_bus_get closes.
    // The function below will undo this behavior:
    dbus_connection_set_exit_on_disconnect(conn, FALSE);
#endif
}

void BTUtils::cleanup() {
    Sockets::cleanup();

#ifndef _WIN32
    // Shut down the connection
    dbus_connection_unref(conn);
    conn = nullptr;
#endif
}

Sockets::DeviceDataList BTUtils::getPaired() {
    Sockets::DeviceDataList deviceList;

#ifdef _WIN32
    // Bluetooth search criteria - only return remembered (paired) devices, and don't start a new inquiry search
    BLUETOOTH_DEVICE_SEARCH_PARAMS searchCriteria{
        .dwSize = sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS),
        .fReturnAuthenticated = false,
        .fReturnRemembered = true,
        .fReturnUnknown = false,
        .fReturnConnected = false,
        .fIssueInquiry = false,
        .cTimeoutMultiplier = 0
    };

    // Find the first device
    BLUETOOTH_DEVICE_INFO_STRUCT deviceInfo{ .dwSize = sizeof(BLUETOOTH_DEVICE_INFO_STRUCT) };
    HBLUETOOTH_DEVICE_FIND foundDevice;

    try {
        foundDevice = EXPECT_TRUE(BluetoothFindFirstDevice, &searchCriteria, &deviceInfo);
    } catch (const System::SystemError& error) {
        if (error.code == ERROR_NO_MORE_ITEMS) return {}; // No paired devices

        throw;
    }

    // Loop through each found device
    do {
        // Format address byte array into MAC address
        const BYTE* addr = deviceInfo.Address.rgBytes;
        std::string mac = std2::format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}",
                                      addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

        // Get the name of the device
        std::string name = Strings::fromSys(deviceInfo.szName);

        // Add to results
        deviceList.emplace_back(Sockets::ConnectionType::None, name, mac, uint16_t{ 0 });
    } while (BluetoothFindNextDevice(foundDevice, &deviceInfo));
#else
    // Set up the method call
    using MsgPtr = HandlePtr<DBusMessage, dbus_message_unref>;

    MsgPtr msg{ EXPECT_TRUE(dbus_message_new_method_call, "org.bluez", "/", "org.freedesktop.DBus.ObjectManager",
                            "GetManagedObjects") };

    MsgPtr response{ EXPECT_TRUE(dbus_connection_send_with_reply_and_block, conn, msg.get(), DBUS_TIMEOUT_USE_DEFAULT,
                                 nullptr) };

    DBusMessageIter responseIter;
    dbus_message_iter_init(response.get(), &responseIter);

    // dbus_message_iter_get_signature(&responseIter) should return "a{oa{sa{sv}}}".
    // Breakdown:
    // a          An array of...
    // {          Dictionaries with...
    //   o        Object paths corresponding to...
    //   a        Arrays of...
    //   {        Dictionaries with...
    //     s      Strings corresponding to...
    //     a      Arrays of...
    //     {      Dictionaries with...
    //       s    Strings corresponding to...
    //       v    Variant types (can hold any datatype)
    //     }
    //   }
    // }
    //
    // https://dbus.freedesktop.org/doc/dbus-specification.html

    // Iterate through the "a{o" part of the signature - array of object paths

    // Array iterator - a
    DBusMessageIter objectsArr;
    do {
        dbus_message_iter_recurse(&responseIter, &objectsArr);

        // Dict iterator - {
        DBusMessageIter objectDict;
        do {
            dbus_message_iter_recurse(&objectsArr, &objectDict);

            // Object path - o
            // If this statement is hit, the object path is found - something like
            // "/org/bluez" or "/org/bluez/hci0/dev_FC_77_74_EB_C1_92".
            // We can ignore this, and move on with the message iterator.
            if (dbus_message_iter_get_arg_type(&objectDict) == DBUS_TYPE_OBJECT_PATH)
                dbus_message_iter_next(&objectDict);

            // Iterate through the a{s part of the signature - array of interfaces

            // Array iterator - a
            DBusMessageIter ifacesArr;
            do {
                dbus_message_iter_recurse(&objectDict, &ifacesArr);

                // Dict iterator - {
                DBusMessageIter ifaceDict;
                do {
                    dbus_message_iter_recurse(&ifacesArr, &ifaceDict);

                    // Interface name - s
                    // Most of what's returned from GetManagedObjects we don't care about,
                    // e.g. org.freedesktop.DBus.Introspectable, org.bluez.LEAdvertisingManager1.
                    // We only want devices (org.bluez.Device1).
                    const char* ifaceName;
                    dbus_message_iter_get_basic(&ifaceDict, &ifaceName);
                    if (std::strcmp(ifaceName, "org.bluez.Device1") != 0) continue;

                    // Advance the iterator to get the next part of the dict
                    dbus_message_iter_next(&ifaceDict);

                    // The following iterators will collect information about each device.
                    // Set up variables to represent each:
                    Sockets::DeviceData device;
                    dbus_bool_t paired = false;

                    // Iterate through the a{s part of the signature - array of properties

                    // Array iterator - a
                    DBusMessageIter propsArr;
                    do {
                        dbus_message_iter_recurse(&ifaceDict, &propsArr);

                        // Dict iterator - {
                        DBusMessageIter propDict;
                        do {
                            dbus_message_iter_recurse(&propsArr, &propDict);

                            // Property name - s
                            const char* propName;
                            dbus_message_iter_get_basic(&propDict, &propName);
                            dbus_message_iter_next(&propDict);

                            // Property value - v
                            // Since this is a variant type, we'll need another message iterator for it.
                            DBusMessageIter propVal;
                            dbus_message_iter_recurse(&propDict, &propVal);

                            // Check if the device is paired (boolean value)
                            if (std::strcmp(propName, "Paired") == 0)
                                dbus_message_iter_get_basic(&propVal, &paired);

                            // Get the name of the device (string value)
                            if (std::strcmp(propName, "Name") == 0) {
                                const char* name;
                                dbus_message_iter_get_basic(&propVal, &name);
                                device.name = name;
                            }

                            // Get the address of the device (string value)
                            if (std::strcmp(propName, "Address") == 0) {
                                const char* address;
                                dbus_message_iter_get_basic(&propVal, &address);
                                device.address = address;
                            }
                        } while (dbus_message_iter_next(&propsArr));
                    } while (dbus_message_iter_next(&ifaceDict));

                    // If the device is paired, add it to the results
                    if (paired) deviceList.push_back(device);
                } while (dbus_message_iter_next(&ifacesArr));
            } while (dbus_message_iter_next(&objectDict));
        } while (dbus_message_iter_next(&objectsArr));
    } while (dbus_message_iter_next(&responseIter));
#endif

    // Done
    return deviceList;
}

#ifdef _WIN32
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
static std::vector<SDP_ELEMENT_DATA> getSDPListData(const LPBLOB blob, USHORT attrib) {
    std::vector<SDP_ELEMENT_DATA> ret;

    // Get the list data by reading from the blob (the SDP stream)
    SDP_ELEMENT_DATA element{};
    EXPECT_ZERO_RC(BluetoothSdpGetAttributeValue, blob->pBlobData, blob->cbSize, attrib, &element);
    return getSDPListData(element);
}
#else
// Converts a Windows-style UUID struct into a Linux-style uuid_t struct.
static uuid_t uuidWindowsToLinux(const UUID& uuid) {
    // Array of 16 bytes to create the UUID used for SDP search
    uint8_t uuidArr[16];

    // A uuid_t on Linux requires fields to be in network byte order, while the fields in a Windows-style UUID struct
    // have a system-dependent byte ordering (endianness). This order is often called the host byte order.
    // The htonl/htons (Host To Network [L]ong/[S]hort) functions are used for this conversion.
    //
    // Data4 does not need a conversion, since it is an array of bytes. Individual bytes are not affected by endianness.
    uint32_t data1 = htonl(uuid.Data1);
    uint16_t data2 = htons(uuid.Data2);
    uint16_t data3 = htons(uuid.Data3);

    // Construct UUID array from passed struct
    std::memcpy(uuidArr, &data1, 4);     // (1) Argument order: Destination, source, size of source in bytes.
    std::memcpy(uuidArr + 4, &data2, 2); // (2) The destination pointer is incremented by the sum of the previous sizes.
    std::memcpy(uuidArr + 6, &data3, 2); // (3) This will ensure that each byte is in the right location in the array.
    std::memcpy(uuidArr + 8, uuid.Data4, 8); // No & operator because Data4 is an array (which decays into a pointer).

    // Create the UUID
    uuid_t ret;
    sdp_uuid128_create(&ret, uuidArr);
    return ret;
}

// Converts a Linux-style uuid_t struct into a Windows-style UUID struct.
static UUID uuidLinuxToWindows(const uuid_t& uuid) {
    // For 16- and 32-bit UUIDs we can derive the rest of the UUID from the Bluetooth base UUID.
    // See the comments inside createUUIDFromBase for details.
    switch (uuid.type) {
    case SDP_UUID16:
        return BTUtils::createUUIDFromBase(uuid.value.uuid16);
    case SDP_UUID32:
        return BTUtils::createUUIDFromBase(uuid.value.uuid32);
    case SDP_UUID128:
        // For a 128-bit UUID we need some more conversions to deal with how both UUID structures are
        UUID ret;
        const uint8_t* data128 = uuid.value.uuid128.data;

        // Copy data into return value
        std::memcpy(&ret.Data1, data128, 4);
        std::memcpy(&ret.Data2, data128 + 4, 2);
        std::memcpy(&ret.Data3, data128 + 6, 2);
        std::memcpy(&ret.Data4, data128 + 8, 8);

        // Make the necessary host byte => network byte ordering conversion
        ret.Data1 = htonl(ret.Data1);
        ret.Data2 = htons(ret.Data2);
        ret.Data3 = htons(ret.Data3);

        return ret;
    }

    // If the type isn't specified, return an empty UUID
    return {};
}
#endif

// Gets the major and minor versions of a Bluetooth profile descriptor.
static void extractVersionNums(uint16_t version, BTUtils::ProfileDesc& desc) {
    // Bit operations to extract the two octets
    // The major and minor version numbers are stored in the high-order 8 bits and low-order 8 bits of the 16-bit
    // version number, respectively. This is the same for both Windows and Linux.
    desc.versionMajor = version >> 8;
    desc.versionMinor = version & 0xFF;
}

BTUtils::SDPResultList BTUtils::sdpLookup(std::string_view addr, UUID uuid, [[maybe_unused]] bool flushCache) {
    // Return value
    SDPResultList ret;

#ifdef _WIN32
    Strings::SysStr addrWide = Strings::toSys(addr);

    // Set up the query set restrictions
    WSAQUERYSET wsaQuery{
        .dwSize = sizeof(WSAQUERYSET),
        .lpServiceClassId = &uuid,
        .dwNameSpace = NS_BTH,
        .lpszContext = addrWide.data(),
        .dwNumberOfCsAddrs = 0
    };

    // Lookup service flags
    DWORD flags = LUP_RETURN_NAME | LUP_RETURN_TYPE | LUP_RETURN_ADDR | LUP_RETURN_BLOB | LUP_RETURN_COMMENT;
    if (flushCache) flags |= LUP_FLUSHCACHE;

    // Start the lookup
    HandlePtr<void, WSALookupServiceEnd> lookup;

    try {
        EXPECT_NONERROR(WSALookupServiceBegin, &wsaQuery, flags, std2::out_ptr(lookup));
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
                result.serviceUUIDs.push_back(data.uuid128);
            }
        }

        // Profile descriptors
        for (auto& element : getSDPListData(wsaResults->lpBlob, SDP_ATTRIB_PROFILE_DESCRIPTOR_LIST)) {
            // Construct a profile descriptor and populate it with the data in the nested container
            ProfileDesc pd;
            for (const auto& [_, specificType, data] : getSDPListData(element)) {
                if (specificType == SDP_ST_UUID16) pd.uuid = data.uuid16;
                else if (specificType == SDP_ST_UINT16) extractVersionNums(data.uint16, pd);
            }

            result.profileDescs.push_back(pd);
        }

        // Add to return vector
        ret.push_back(result);
    }
#else
    // TODO: Use std::bind_back() instead of lambda in C++23
    using SDPListPtr = HandlePtr<sdp_list_t, [](sdp_list_t* p) { sdp_list_free(p, nullptr); }>;

    // Parse the MAC address into a Bluetooth address structure
    bdaddr_t bdAddr;
    str2ba(addr.data(), &bdAddr);

    // Initialize SDP session
    // We can't directly pass BDADDR_ANY to sdp_connect() because a "taking the address of an rvalue" error is thrown.
    bdaddr_t addrAny{};
    HandlePtr<sdp_session_t, sdp_close> session{ EXPECT_TRUE(sdp_connect, &addrAny, &bdAddr, SDP_RETRY_IF_BUSY) };

    uuid_t serviceUUID = uuidWindowsToLinux(uuid);

    // Start SDP service search
    SDPListPtr searchList{ sdp_list_append(nullptr, &serviceUUID) };
    SDPListPtr responseList;

    uint32_t range = 0x0000FFFF;
    SDPListPtr attridList{ sdp_list_append(nullptr, &range) };

    EXPECT_NONERROR(sdp_service_search_attr_req, session.get(), searchList.get(), SDP_ATTR_REQ_RANGE, attridList.get(),
                    std2::out_ptr(responseList));

    // Iterate through each of the service records
    for (sdp_list_t* r = responseList.get(); r; r = r->next) {
        SDPResult result;

        HandlePtr<sdp_record_t, sdp_record_free> rec{ static_cast<sdp_record_t*>(r->data) };
        SDPListPtr protoList;

        // Get a list of the protocol sequences
        if (sdp_get_access_protos(rec.get(), std2::out_ptr(protoList)) == SOCKET_ERROR) continue;

        // Iterate through each protocol sequence
        for (sdp_list_t* p = protoList.get(); p; p = p->next) {
            // Iterate through each protocol list of the protocol sequence
            for (SDPListPtr pds{ static_cast<sdp_list_t*>(p->data) }; pds; pds.reset(pds->next)) {
                // Check protocol attributes
                uint16_t proto = 0;
                for (sdp_data_t* d = static_cast<sdp_data_t*>(pds->data); d; d = d->next) {
                    switch (d->dtd) {
                    case SDP_UUID16:
                    case SDP_UUID32:
                    case SDP_UUID128:
                        // Keep track of protocol UUIDs
                        proto = sdp_uuid_to_proto(&d->val.uuid);
                        result.protoUUIDs.push_back(proto);
                        break;
                    case SDP_UINT8:
                        // RFCOMM channel is stored in an 8-bit integer
                        if (proto == RFCOMM_UUID) result.port = d->val.int8;
                        break;
                    case SDP_UINT16:
                        // L2CAP PSM is stored in a 16-bit integer
                        if (proto == L2CAP_UUID) result.port = d->val.int16;
                    }
                }
            }
        }

        // Get the list of service class IDs
        SDPListPtr svClassList;
        if (sdp_get_service_classes(rec.get(), std2::out_ptr(svClassList)) == NO_ERROR) {
            for (sdp_list_t* iter = svClassList.get(); iter; iter = iter->next)
                result.serviceUUIDs.push_back(uuidLinuxToWindows(*static_cast<uuid_t*>(iter->data)));
        }

        // Get the list of profile descriptors
        SDPListPtr profileDescList;
        if (sdp_get_profile_descs(rec.get(), std2::out_ptr(profileDescList)) == NO_ERROR) {
            for (sdp_list_t* iter = profileDescList.get(); iter; iter = iter->next) {
                sdp_profile_desc_t* desc = static_cast<sdp_profile_desc_t*>(iter->data);

                // Extract information
                ProfileDesc pd;
                pd.uuid = desc->uuid.value.uuid16;
                extractVersionNums(desc->version, pd);

                result.profileDescs.push_back(pd);
            }
        }

        // Allocate 1 kb for name/desc strings
        int strBufLen = 1024;
        result.name.resize(strBufLen);
        result.desc.resize(strBufLen);

        // Get the relevant strings
        // (Clear a string field if it can't be retreived to ensure it's empty)
        // TODO: Change code to resize strings so trailing null terminators are removed
        if (sdp_get_service_name(rec.get(), result.name.data(), strBufLen) == SOCKET_ERROR) result.name.clear();
        if (sdp_get_service_desc(rec.get(), result.desc.data(), strBufLen) == SOCKET_ERROR) result.desc.clear();

        // Add to return vector
        ret.push_back(result);
    }
#endif

    // Return result
    return ret;
}
