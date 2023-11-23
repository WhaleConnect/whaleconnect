// Copyright 2021-2023 Aidan Sun and the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

module;
#if OS_LINUX
#include <array>
#include <functional>
#include <stdexcept>
#include <string_view>

#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <dbus/dbus.h>
#include <ztd/out_ptr.hpp>

#include "os/check.hpp"

module net.btutils;
import net.device;
import net.btutils.internal;
import os.errcheck;
import utils.handleptr;
import utils.strings;

DBusConnection* conn = nullptr;

void recurseDictIter(DBusMessageIter& dictIter, auto fn) {
    do {
        DBusMessageIter iter;
        dbus_message_iter_recurse(&dictIter, &iter);

        do {
            DBusMessageIter sub;
            dbus_message_iter_recurse(&iter, &sub);
            fn(sub);
        } while (dbus_message_iter_next(&iter));
    } while (dbus_message_iter_next(&dictIter));
}

void getDeviceInfo(dbus_bool_t& paired, Device& device, DBusMessageIter& propDict) {
    using namespace std::literals;

    // Property name - s
    const char* propName;
    dbus_message_iter_get_basic(&propDict, &propName);
    dbus_message_iter_next(&propDict);

    // Property value - v
    // Since this is a variant type, we'll need another message iterator for it.
    DBusMessageIter propVal;
    dbus_message_iter_recurse(&propDict, &propVal);

    // Check if the device is paired (boolean value)
    if (propName == "Paired"sv) dbus_message_iter_get_basic(&propVal, &paired);

    // Get the name of the device (string value)
    if (propName == "Name"sv) {
        const char* name;
        dbus_message_iter_get_basic(&propVal, &name);
        device.name = name;
    }

    // Get the address of the device (string value)
    if (propName == "Address"sv) {
        const char* address;
        dbus_message_iter_get_basic(&propVal, &address);
        device.address = address;
    }
}

void checkProtocolAttributes(sdp_list_t* pds, BTUtils::SDPResult& result) {
    uint16_t proto = 0;
    for (auto d = static_cast<sdp_data_t*>(pds->data); d; d = d->next) {
        switch (d->dtd) {
            case SDP_UUID16:
            case SDP_UUID32:
            case SDP_UUID128:
                // Keep track of protocol UUIDs
                proto = static_cast<uint16_t>(sdp_uuid_to_proto(&d->val.uuid));
                result.protoUUIDs.push_back(proto);
                break;
            case SDP_UINT8:
                // RFCOMM channel is stored in an 8-bit integer
                if (proto == RFCOMM_UUID) result.port = d->val.int8;
                break;
            case SDP_UINT16:
                // L2CAP PSM is stored in a 16-bit integer
                if (proto == L2CAP_UUID) result.port = d->val.int16;
                break;
            default:
                // Other values not handled
                break;
        }
    }
}

BTUtils::UUID128 getUUID(uuid_t* uuid) {
    switch (uuid->type) {
        case SDP_UUID16:
            return BTUtils::createUUIDFromBase(uuid->value.uuid16);
        case SDP_UUID32:
            return BTUtils::createUUIDFromBase(uuid->value.uuid32);
        case SDP_UUID128:
            return std::to_array(uuid->value.uuid128.data);
        default:
            throw std::invalid_argument{ "Unknown UUID type" };
    }
}

BTUtils::Instance::Instance() {
    // Connect to the system D-Bus
    conn = CHECK(dbus_bus_get(DBUS_BUS_SYSTEM, nullptr), checkTrue);

    // The process will exit when a connection with dbus_bus_get closes.
    // The function below will undo this behavior:
    dbus_connection_set_exit_on_disconnect(conn, FALSE);
}

BTUtils::Instance::~Instance() {
    // Shut down the connection
    dbus_connection_unref(conn);
    conn = nullptr;
}

DeviceList BTUtils::getPaired() {
    DeviceList deviceList;

    if (!conn) return deviceList;

    // Set up the method call
    using MsgPtr = HandlePtr<DBusMessage, dbus_message_unref>;

    MsgPtr msg{ CHECK(
        dbus_message_new_method_call("org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects"),
        checkTrue) };

    MsgPtr response{ CHECK(
        dbus_connection_send_with_reply_and_block(conn, msg.get(), DBUS_TIMEOUT_USE_DEFAULT, nullptr), checkTrue) };

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
    using namespace std::literals;
    recurseDictIter(responseIter, [&deviceList](DBusMessageIter& objectDict) {
        // Object path - o
        // If this statement is hit, the object path is found - something like
        // "/org/bluez" or "/org/bluez/hci0/dev_FC_77_74_EB_C1_92".
        // We can ignore this, and move on with the message iterator.
        if (dbus_message_iter_get_arg_type(&objectDict) == DBUS_TYPE_OBJECT_PATH) dbus_message_iter_next(&objectDict);

        // Iterate through the a{s part of the signature - array of interfaces
        recurseDictIter(objectDict, [&deviceList](DBusMessageIter& ifaceDict) {
            // Interface name - s
            // Most of what's returned from GetManagedObjects we don't care about,
            // e.g. org.freedesktop.DBus.Introspectable, org.bluez.LEAdvertisingManager1.
            // We only want devices (org.bluez.Device1).
            const char* ifaceName;
            dbus_message_iter_get_basic(&ifaceDict, &ifaceName);
            if (ifaceName != "org.bluez.Device1"sv) return;

            // Advance the iterator to get the next part of the dict
            dbus_message_iter_next(&ifaceDict);

            // The following iterators will collect information about each device.
            // Set up variables to represent each:
            Device device;
            dbus_bool_t paired = false;

            // Iterate through the a{s part of the signature - array of properties
            recurseDictIter(ifaceDict, std::bind_front(getDeviceInfo, std::ref(paired), std::ref(device)));

            // If the device is paired, add it to the results
            if (paired) deviceList.push_back(device);
        });
    });

    return deviceList;
}

BTUtils::SDPResultList BTUtils::sdpLookup(std::string_view addr, UUID128 uuid, bool) {
    SDPResultList ret;

    // TODO: Use std::bind_back() instead of lambda in C++23
    using SDPListPtr = HandlePtr<sdp_list_t, [](sdp_list_t* p) { sdp_list_free(p, nullptr); }>;

    // Parse the MAC address into a Bluetooth address structure
    bdaddr_t bdAddr;
    str2ba(addr.data(), &bdAddr);

    // Initialize SDP session
    // We can't directly pass BDADDR_ANY to sdp_connect() because a "taking the address of an rvalue" error is thrown.
    bdaddr_t addrAny{};
    HandlePtr<sdp_session_t, sdp_close> session{ CHECK(sdp_connect(&addrAny, &bdAddr, SDP_RETRY_IF_BUSY), checkTrue) };

    uuid_t serviceUUID;
    sdp_uuid128_create(&serviceUUID, uuid.data());

    // Start SDP service search
    SDPListPtr searchList{ sdp_list_append(nullptr, &serviceUUID) };
    SDPListPtr responseList;

    uint32_t range = 0x0000FFFF;
    SDPListPtr attridList{ sdp_list_append(nullptr, &range) };

    CHECK(sdp_service_search_attr_req(session.get(), searchList.get(), SDP_ATTR_REQ_RANGE, attridList.get(),
        ztd::out_ptr::out_ptr(responseList)));

    // Iterate through each of the service records
    for (sdp_list_t* r = responseList.get(); r; r = r->next) {
        HandlePtr<sdp_record_t, sdp_record_free> rec{ static_cast<sdp_record_t*>(r->data) };
        SDPResult result;

        // Allocate 1 kb for name/desc strings
        int strBufLen = 1024;
        result.name.resize(strBufLen);
        result.desc.resize(strBufLen);
        sdp_get_service_name(rec.get(), result.name.data(), strBufLen);
        sdp_get_service_desc(rec.get(), result.desc.data(), strBufLen);

        // Erase null chars
        Strings::stripNull(result.name);
        Strings::stripNull(result.desc);

        // Free function for the list returned from sdp_get_access_protos (inner lists must also be freed)
        auto protoListFreeFn = [](sdp_list_t* p) {
            for (auto list = p; list; list = list->next) sdp_list_free(static_cast<sdp_list_t*>(list->data), nullptr);

            sdp_list_free(p, nullptr);
        };

        // Get a list of the protocol sequences
        HandlePtr<sdp_list_t, protoListFreeFn> protoList;
        if (sdp_get_access_protos(rec.get(), ztd::out_ptr::out_ptr(protoList)) == -1) continue;

        // Iterate through each protocol sequence, then through each protocol list
        for (auto p = protoList.get(); p; p = p->next)
            for (auto pds = static_cast<sdp_list_t*>(p->data); pds; pds = pds->next)
                checkProtocolAttributes(pds, result);

        // Get the list of service class IDs
        SDPListPtr svClassList;
        if (sdp_get_service_classes(rec.get(), ztd::out_ptr::out_ptr(svClassList)) == 0)
            for (auto iter = svClassList.get(); iter; iter = iter->next)
                result.serviceUUIDs.push_back(getUUID(static_cast<uuid_t*>(iter->data)));

        // Get the list of profile descriptors
        SDPListPtr profileDescList;
        if (sdp_get_profile_descs(rec.get(), ztd::out_ptr::out_ptr(profileDescList)) == 0) {
            for (auto iter = profileDescList.get(); iter; iter = iter->next) {
                auto desc = static_cast<sdp_profile_desc_t*>(iter->data);

                // Extract information
                ProfileDesc pd;
                pd.uuid = desc->uuid.value.uuid16;
                Internal::extractVersionNums(desc->version, pd);

                result.profileDescs.push_back(pd);
            }
        }

        // Add to return vector
        ret.push_back(result);
    }

    return ret;
}
#endif
