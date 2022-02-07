// Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "btutils.hpp"
#include "sockets.hpp"

#ifdef _WIN32
#include <memory> // std::make_unique()
#include <format>

#include <WinSock2.h>
#include <ws2bth.h>
#include <bluetoothapis.h>

#include "sys/error.hpp"
#include "util/strings.hpp"

// Link with Bluetooth lib
#pragma comment(lib, "Bthprops.lib")

static bool wsInitialized = false;
#else
#include <cstring> // std::memcpy()

#include <dbus/dbus.h>

#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

static DBusConnection* conn = nullptr;
#endif

int BTUtils::init() {
    if (initialized()) return NO_ERROR;

#ifdef _WIN32
    // Initialize Winsock
    int ret = Sockets::init();
    wsInitialized = (ret == NO_ERROR);
    return ret;
#else
    // Connect to the system D-Bus
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, nullptr);
    if (conn) {
        // The process will exit when a connection with `dbus_bus_get` closes.
        // The function below will undo this behavior:
        dbus_connection_set_exit_on_disconnect(conn, FALSE);
        return NO_ERROR;
    }
    return SOCKET_ERROR;
#endif
}

void BTUtils::cleanup() {
    if (!initialized()) return;

#ifdef _WIN32
    if (Sockets::cleanup() == NO_ERROR) wsInitialized = false;
#else
    // Shut down the connection
    dbus_connection_unref(conn);
    conn = nullptr;
#endif
}

bool BTUtils::initialized() {
#ifdef _WIN32
    return wsInitialized;
#else
    return conn && dbus_connection_get_is_connected(conn);
#endif
}

int BTUtils::getPaired(Sockets::DeviceDataList& deviceList) {
    // Clear the vector
    deviceList.clear();

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
    HBLUETOOTH_DEVICE_FIND foundDevice = BluetoothFindFirstDevice(&searchCriteria, &deviceInfo);

    // Error occurred and this pointer is null.
    if (!foundDevice) {
        if (System::getLastErr() == ERROR_NO_MORE_ITEMS) {
            // This indicates that no Bluetooth devices are paired and is not considered a failure.
            System::setLastErr(NO_ERROR);
            return NO_ERROR;
        } else {
            // Other errors are fatal, return a `SOCKET_ERROR`.
            return SOCKET_ERROR;
        }
    }

    // Loop through each found device
    do {
        // Format address byte array into MAC address
        const BYTE* addr = deviceInfo.Address.rgBytes;
        std::string mac = std::format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}",
                                      addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

        // Get the name of the device
        std::string name = Strings::fromWide(deviceInfo.szName);

        // Add to results
        deviceList.emplace_back(Sockets::ConnectionType::None, name, mac, 0);
    } while (BluetoothFindNextDevice(foundDevice, &deviceInfo));
#else
    // Set up the method call
    DBusMessage* msg = dbus_message_new_method_call("org.bluez",
                                                    "/",
                                                    "org.freedesktop.DBus.ObjectManager",
                                                    "GetManagedObjects");
    if (!msg) return SOCKET_ERROR;

    DBusMessage* response = dbus_connection_send_with_reply_and_block(conn, msg, DBUS_TIMEOUT_USE_DEFAULT, nullptr);
    if (!response) return SOCKET_ERROR;

    DBusMessageIter responseIter;
    dbus_message_iter_init(response, &responseIter);

    // `dbus_message_iter_get_signature(&responseIter)` should return "a{oa{sa{sv}}}".
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

    // Iterate through the `a{o` part of the signature - array of object paths

    // Array iterator - `a`
    DBusMessageIter objectsArr;
    do {
        dbus_message_iter_recurse(&responseIter, &objectsArr);

        // Dict iterator - `{`
        DBusMessageIter objectDict;
        do {
            dbus_message_iter_recurse(&objectsArr, &objectDict);

            // Object path - `o`
            // If this statement is hit, the object path is found - something like
            // "/org/bluez" or "/org/bluez/hci0/dev_FC_77_74_EB_C1_92".
            // We can ignore this, and move on with the message iterator.
            if (dbus_message_iter_get_arg_type(&objectDict) == DBUS_TYPE_OBJECT_PATH)
                dbus_message_iter_next(&objectDict);

            // Iterate through the `a{s` part of the signature - array of interfaces

            // Array iterator - `a`
            DBusMessageIter ifacesArr;
            do {
                dbus_message_iter_recurse(&objectDict, &ifacesArr);

                // Dict iterator - `{`
                DBusMessageIter ifaceDict;
                do {
                    dbus_message_iter_recurse(&ifacesArr, &ifaceDict);

                    // Interface name - `s`
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

                    // Iterate through the `a{s` part of the signature - array of properties

                    // Array iterator - `a`
                    DBusMessageIter propsArr;
                    do {
                        dbus_message_iter_recurse(&ifaceDict, &propsArr);

                        // Dict iterator - `{`
                        DBusMessageIter propDict;
                        do {
                            dbus_message_iter_recurse(&propsArr, &propDict);

                            // Property name - `s`
                            const char* propName;
                            dbus_message_iter_get_basic(&propDict, &propName);
                            dbus_message_iter_next(&propDict);

                            // Property value - `v`
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

    dbus_message_unref(msg);
    dbus_message_unref(response);
#endif

    // Done
    return NO_ERROR;
}

#ifdef _WIN32
/// <summary>
/// Get the SDP container data in a given SDP element.
/// </summary>
/// <param name="element">The SDP_ELEMENT_DATA which contains the SDP stream to read from</param>
/// <returns>The container data as a vector</returns>
static std::vector<SDP_ELEMENT_DATA> getSDPListData(SDP_ELEMENT_DATA& element) {
    std::vector<SDP_ELEMENT_DATA> ret;

    LPBYTE value = element.data.sequence.value;
    ULONG length = element.data.sequence.length;

    HBLUETOOTH_CONTAINER_ELEMENT iter = nullptr;
    while (BluetoothSdpGetContainerElementData(value, length, &iter, &element) == ERROR_SUCCESS) ret.push_back(element);

    return ret;
}

/// <summary>
/// Get the SDP container data associated with a given SDP attribute.
/// </summary>
/// <param name="blob">The SDP stream to read from</param>
/// <param name="attrib">The attribute to get the data from</param>
/// <returns>The container data as a vector</returns>
static std::vector<SDP_ELEMENT_DATA> getSDPListData(const LPBLOB blob, USHORT attrib) {
    std::vector<SDP_ELEMENT_DATA> ret;

    // Get the list data
    SDP_ELEMENT_DATA element{};
    DWORD getValueRet = BluetoothSdpGetAttributeValue(blob->pBlobData, blob->cbSize, attrib, &element);

    if (getValueRet != ERROR_SUCCESS) return ret;
    return getSDPListData(element);
}
#else
/// <summary>
/// Convert a Windows-style UUID struct into a Linux-style uuid_t struct.
/// </summary>
/// <param name="uuid">The UUID struct to convert</param>
/// <returns>
/// The converted uuid_t struct
/// </returns>
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
    std::memcpy(uuidArr + 8, uuid.Data4, 8); // No `&` operator because Data4 is an array (which decays into a pointer).

    // Create the UUID
    uuid_t ret;
    sdp_uuid128_create(&ret, uuidArr);
    return ret;
}

/// <summary>
/// Convert a Linux-style uuid_t struct into a Windows-style UUID struct.
/// </summary>
/// <param name="uuid">The uuid_t struct to convert</param>
/// <returns>
/// The converted UUID struct
/// </returns>
static UUID uuidLinuxToWindows(const uuid_t& uuid) {
    // For 16- and 32-bit UUIDs we can derive the rest of the UUID from the Bluetooth base UUID.
    // (See the comments inside `createUUIDFromBase` for details.)
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

/// <summary>
/// Get the major and minor versions of a Bluetooth profile descriptor.
/// </summary>
/// <param name="version">The 16-bit integer containing both version numbers</param>
/// <param name="desc">The structure where the extracted values will be placed</param>
/// <remarks>
/// The major and minor versions are stored in the high-order 8 bits and low-order 8 bits of the 16-bit version number,
/// respectively. This is the same for both Windows and Linux.
/// </remarks>
static void extractVersionNums(uint16_t version, BTUtils::ProfileDesc& desc) {
    // Bit operations to extract the two octets
    desc.versionMajor = version >> 8;
    desc.versionMinor = version & 0xFF;
}

BTUtils::SDPResultList BTUtils::sdpLookup(std::string_view addr, const UUID& uuid, [[maybe_unused]] bool flushCache) {
    // Return value
    SDPResultList ret;

#ifdef _WIN32
    Strings::WideStr addrWide = Strings::toWide(addr);

    // Set up the query set restrictions
    WSAQUERYSET wsaQuery{
        .dwSize = sizeof(WSAQUERYSET),
        .lpServiceClassId = const_cast<LPUUID>(&uuid),
        .dwNameSpace = NS_BTH,
        .lpszContext = addrWide.data(),
        .dwNumberOfCsAddrs = 0
    };

    // Lookup service flags
    DWORD flags = LUP_RETURN_NAME | LUP_RETURN_TYPE | LUP_RETURN_ADDR | LUP_RETURN_BLOB | LUP_RETURN_COMMENT;
    if (flushCache) flags |= LUP_FLUSHCACHE;

    // Start the lookup
    HANDLE hLookup = nullptr;
    if (WSALookupServiceBegin(&wsaQuery, flags, &hLookup) == SOCKET_ERROR) return ret;

    // Continue the lookup
    DWORD size = 2048;
    auto resultsBuf = std::make_unique<BYTE[]>(size);
    LPWSAQUERYSET wsaResults = reinterpret_cast<LPWSAQUERYSET>(resultsBuf.get());
    wsaResults->dwSize = size;
    wsaResults->dwNameSpace = NS_BTH;

    // Get various service information
    while (WSALookupServiceNext(hLookup, flags, &size, wsaResults) == NO_ERROR) {
        SDPResult result;

        // The name and description
        result.name = Strings::fromWide(wsaResults->lpszServiceInstanceName);
        result.desc = Strings::fromWide(wsaResults->lpszComment);

        // Protocol descriptors
        auto protoList = getSDPListData(wsaResults->lpBlob, SDP_ATTRIB_PROTOCOL_DESCRIPTOR_LIST);
        if (protoList.empty()) continue;

        // Iterate through the list of protocol descriptors (includes UUIDs + port)
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
        auto svClassList = getSDPListData(wsaResults->lpBlob, SDP_ATTRIB_CLASS_ID_LIST);
        if (svClassList.empty()) continue;

        // Iterate through the list of service class IDs
        for (const auto& [_, specificType, data] : svClassList) {
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
        auto profileList = getSDPListData(wsaResults->lpBlob, SDP_ATTRIB_PROFILE_DESCRIPTOR_LIST);
        if (profileList.empty()) continue;

        // Iterate through the list of profile descriptors
        for (auto& element : profileList) {
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

    // End the lookup
    WSALookupServiceEnd(hLookup);
#else
    // Parse the MAC address into a Bluetooth address structure
    bdaddr_t deviceAddr;
    str2ba(addr.c_str(), &deviceAddr);

    // Initialize SDP session
    // We can't directly pass BDADDR_ANY to sdp_connect() because a "taking the address of an rvalue" error is thrown.
    bdaddr_t addrAny{};
    sdp_session_t* session = sdp_connect(&addrAny, &deviceAddr, SDP_RETRY_IF_BUSY);
    if (!session) return ret; // Failed to connect to SDP session

    uuid_t serviceUUID = uuidWindowsToLinux(uuid);

    // Start SDP service search
    sdp_list_t* searchList = sdp_list_append(nullptr, &serviceUUID);
    sdp_list_t* responseList = nullptr;

    uint32_t range = 0x0000FFFF;
    sdp_list_t* attridList = sdp_list_append(nullptr, &range);

    int err = sdp_service_search_attr_req(session, searchList, SDP_ATTR_REQ_RANGE, attridList, &responseList);
    sdp_close(session);

    if (err == SOCKET_ERROR) {
        // Failed to initialize service search, free all lists and return
        sdp_list_free(responseList, nullptr);
        sdp_list_free(searchList, nullptr);
        sdp_list_free(attridList, nullptr);
        return ret;
    }

    // Iterate through each of the service records
    for (sdp_list_t* r = responseList; r; r = r->next) {
        SDPResult result;

        sdp_record_t* rec = static_cast<sdp_record_t*>(r->data);
        sdp_list_t* protoList;

        // Get a list of the protocol sequences
        if (sdp_get_access_protos(rec, &protoList) == NO_ERROR) {
            // Iterate through each protocol sequence
            for (sdp_list_t* p = protoList; p; p = p->next) {
                // Iterate through each protocol list of the protocol sequence
                for (sdp_list_t* pds = static_cast<sdp_list_t*>(p->data); pds; pds = pds->next) {
                    // Check protocol attributes
                    int proto = 0;
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
                sdp_list_free(static_cast<sdp_list_t*>(p->data), nullptr);
            }
            sdp_list_free(protoList, nullptr);
        } else {
            continue;
        }

        // Get the list of service class IDs
        sdp_list_t* svClassList;
        if (sdp_get_service_classes(rec, &svClassList) == NO_ERROR) {
            for (sdp_list_t* iter = svClassList; iter; iter = iter->next)
                result.serviceUUIDs.push_back(uuidLinuxToWindows(*static_cast<uuid_t*>(iter->data)));

            sdp_list_free(svClassList, nullptr);
        } else {
            continue;
        }

        // Get the list of profile descriptors
        sdp_list_t* profileDescList;
        if (sdp_get_profile_descs(rec, &profileDescList) == NO_ERROR) {
            for (sdp_list_t* iter = profileDescList; iter; iter = iter->next) {
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
        if (sdp_get_service_name(rec, result.name.data(), strBufLen) == SOCKET_ERROR) result.name.clear();
        if (sdp_get_service_desc(rec, result.desc.data(), strBufLen) == SOCKET_ERROR) result.desc.clear();

        // Add to return vector, then free SDP record
        ret.push_back(result);
        sdp_record_free(rec);
    }

    sdp_list_free(responseList, nullptr);
    sdp_list_free(searchList, nullptr);
    sdp_list_free(attridList, nullptr);
#endif

    // Return result
    return ret;
}
