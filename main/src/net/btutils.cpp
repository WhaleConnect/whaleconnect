// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Much of the Linux GDBus code has been modified from the bluetoothctl source code:
//     https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/client/main.c
//
// The original code has the following license and copyright statements:
//     SPDX-License-Identifier: GPL-2.0-or-later
//
//     BlueZ - Bluetooth protocol stack for Linux
//
//     Copyright (C) 2012  Intel Corporation. All rights reserved.
//
// Some notes about the modified code:
//     The majority of the GDBus functions are lacking of comments because the original bluetoothctl sources and the
//     DBus/GDBus APIs are all poorly documented.
//
//     std::strcmp() is used instead of std::strncmp(). This is because the original code uses strcmp(), and since all
//     strings are internally-supplied valid (i.e. null-terminated) C-strings the function is guaranteed not to overrun
//     the buffer.
//
//     strcasecmp() is used in the proxyAdded() function. This is not part of the C/C++ standard, but rather the POSIX
//     standard.

#include <cstring> // std::strcmp(), std::snprintf()

#include "btutils.hpp"
#include "sockets.hpp"

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2bth.h>
#include <bluetoothapis.h>

#include "util/strings.hpp"

// Link with Bluetooth lib
#pragma comment(lib, "Bthprops.lib")
#else
#include <cerrno> // ENODEV

#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <gdbus/gdbus.h>

// A Bluetooth adapter
struct adapter {
    GDBusProxy* proxy;
    GList* devices;
};

static adapter* defaultCtrl;
static GDBusProxy* defaultDev;
static GList* ctrlList;

static DBusConnection* dbusConn;
static GDBusClient* client;
static bool dbusConnected = false;

static void connectHandler(DBusConnection*, void*) {
    // Once connected, notify the rest of the app by setting this flag
    dbusConnected = true;
}

static void disconnectHandler(DBusConnection*, void*) {
    // Once disconnected, free all lists and allocated resources
    g_list_free(ctrlList);
    ctrlList = nullptr;
    defaultCtrl = nullptr;
    dbusConnected = false;
}

static gboolean deviceIsChild(GDBusProxy* device, GDBusProxy* master) {
    if (!master) return false;

    DBusMessageIter iter;
    if (!g_dbus_proxy_get_property(device, "Adapter", &iter)) return false;

    const char* adapter;
    dbus_message_iter_get_basic(&iter, &adapter);
    return std::strcmp(g_dbus_proxy_get_path(master), adapter) == 0;
}

static adapter* findParent(GDBusProxy* device) {
    for (GList* list = g_list_first(ctrlList); list; list = g_list_next(list)) {
        adapter* a = reinterpret_cast<adapter*>(list->data);
        if (deviceIsChild(device, a->proxy)) return a;
    }
    return nullptr;
}

static void proxyAdded(GDBusProxy* proxy, void*) {
    const char* interface = g_dbus_proxy_get_interface(proxy);
    if (std::strcmp(interface, "org.bluez.Device1") == 0) {
        adapter* a = findParent(proxy);
        if (!a) return;
        a->devices = g_list_append(a->devices, proxy);
        if (defaultDev) return;

        DBusMessageIter iter;
        if (g_dbus_proxy_get_property(proxy, "Connected", &iter)) {
            dbus_bool_t connected;
            dbus_message_iter_get_basic(&iter, &connected);
            if (connected) defaultDev = proxy;
        }
    } else if (std::strcmp(interface, "org.bluez.Adapter1") == 0) {
        adapter* a = nullptr;
        for (GList* list = g_list_first(ctrlList); list; list = g_list_next(list)) {
            adapter* tmp = reinterpret_cast<adapter*>(list->data);
            if (!strcasecmp(g_dbus_proxy_get_path(tmp->proxy), g_dbus_proxy_get_path(proxy))) {
                a = tmp;
                break;
            }
        }

        if (!a) a = reinterpret_cast<adapter*>(g_malloc0(sizeof(adapter)));
        ctrlList = g_list_append(ctrlList, a);
        if (!defaultCtrl) defaultCtrl = a;
        a->proxy = proxy;
    }
}

static void proxyRemoved(GDBusProxy* proxy, void*) {
    const char* interface = g_dbus_proxy_get_interface(proxy);
    if (std::strcmp(interface, "org.bluez.Device1") == 0) {
        adapter* a = findParent(proxy);
        if (!a) return;
        a->devices = g_list_remove(a->devices, proxy);
        if (defaultDev == proxy) defaultDev = nullptr;
    } else if (std::strcmp(interface, "org.bluez.Adapter1") == 0) {
        for (GList* ll = g_list_first(ctrlList); ll; ll = g_list_next(ll)) {
            adapter* a = reinterpret_cast<adapter*>(ll->data);

            if (a->proxy == proxy) {
                if (defaultCtrl && defaultCtrl->proxy == proxy) {
                    defaultCtrl = nullptr;
                    defaultDev = nullptr;
                }

                ctrlList = g_list_remove_link(ctrlList, ll);
                g_list_free(a->devices);
                g_free(a);
                g_list_free(ll);
                return;
            }
        }
    }
}

static void propertyChanged(GDBusProxy* proxy, const char* name, DBusMessageIter* iter, void*) {
    bool isDevice1 = (std::strcmp(g_dbus_proxy_get_interface(proxy), "org.bluez.Device1") == 0);
    bool connected = (std::strcmp(name, "Connected") == 0);
    if (isDevice1 && connected && defaultCtrl && deviceIsChild(proxy, defaultCtrl->proxy)) {
        dbus_bool_t connected;
        dbus_message_iter_get_basic(iter, &connected);

        if (connected && defaultDev == nullptr) defaultDev = proxy;
        else if (!connected && defaultDev == proxy) defaultDev = nullptr;
    }
}

void BTUtils::glibInit() {
    // Create a new connection and client to bluetoothd
    dbusConn = g_dbus_setup_bus(DBUS_BUS_SYSTEM, nullptr, nullptr);
    g_dbus_attach_object_manager(dbusConn);
    client = g_dbus_client_new(dbusConn, "org.bluez", "/org/bluez");

    // Set up client event handlers
    g_dbus_client_set_connect_watch(client, connectHandler, nullptr);
    g_dbus_client_set_disconnect_watch(client, disconnectHandler, nullptr);
    g_dbus_client_set_proxy_handlers(client, proxyAdded, proxyRemoved, propertyChanged, nullptr);
}

void BTUtils::glibStop() {
    // Shut down the client and DBus connection
    g_dbus_client_unref(client);
    dbus_connection_unref(dbusConn);
    g_list_free(ctrlList);
}
#endif

bool BTUtils::initialized() {
#ifdef _WIN32
    return true;
#else
    return dbusConnected;
#endif
}

void BTUtils::glibMainContextIteration() {
#ifndef _WIN32
    // The below line of code is used to poll GLib events without using its main loop facilities.
    // https://stackoverflow.com/a/64580893 (See comments)
    g_main_context_iteration(g_main_context_default(), false);
#endif
}

int BTUtils::getPaired(std::vector<Sockets::DeviceData>& deviceList) {
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
    BLUETOOTH_DEVICE_INFO deviceInfo{ .dwSize = sizeof(BLUETOOTH_DEVICE_INFO) };
    HBLUETOOTH_DEVICE_FIND foundDevice = BluetoothFindFirstDevice(&searchCriteria, &deviceInfo);

    // Error 259 ("no more data") indicates that no Bluetooth devices are paired.
    // This can be handled with a `return NO_ERROR`.
    // Other errors are fatal, return a `SOCKET_ERROR`.
    //
    // WSAGetLastError() and GetLastError() are the same. The latter is typically used more outside of Winsock
    // programming like in this function. https://stackoverflow.com/a/15586393
    if (!foundDevice) return (GetLastError() == 259) ? NO_ERROR : SOCKET_ERROR;

    // Loop through each found device
    do {
        // Device MAC address broken into six octets
        BYTE* addr = deviceInfo.Address.rgBytes;

        // Format address array into MAC address
        constexpr size_t len = 18;
        char mac[len];
        std::snprintf(mac, len, "%02X:%02X:%02X:%02X:%02X:%02X", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

        // Push back
        std::string name = Strings::fromWide(deviceInfo.szName);
        deviceList.emplace_back(Sockets::ConnectionType::Bluetooth, name, mac, 0, deviceInfo.Address.ullLong);
    } while (BluetoothFindNextDevice(foundDevice, &deviceInfo));
#else
    if (!defaultCtrl) {
        errno = ENODEV;
        return SOCKET_ERROR;
    }

    for (GList* ll = g_list_first(defaultCtrl->devices); ll; ll = g_list_next(ll)) {
        GDBusProxy* proxy = reinterpret_cast<GDBusProxy*>(ll->data);

        // Get the paired devices
        DBusMessageIter iter;
        if (!g_dbus_proxy_get_property(proxy, "Paired", &iter)) continue;

        // Is the device paired?
        dbus_bool_t paired;
        dbus_message_iter_get_basic(&iter, &paired);
        if (!paired) continue;

        // Get the address of the device
        const char* address;
        if (!g_dbus_proxy_get_property(proxy, "Address", &iter)) continue;
        dbus_message_iter_get_basic(&iter, &address);

        // Get the name of the device
        const char* name;
        if (!g_dbus_proxy_get_property(proxy, "Alias", &iter)) continue;
        dbus_message_iter_get_basic(&iter, &name);

        // Push back
        deviceList.emplace_back(Sockets::ConnectionType::Bluetooth, name, address, 0, 0);
    }
#endif

    // The enumeration of devices was successful
    return NO_ERROR;
}

uint8_t BTUtils::getSDPChannel(const char* addr) {
    // Return value
    uint8_t ret = 0;

#ifdef _WIN32
    Strings::widestr tmp = Strings::toWide(addr);
    LPWSTR addrWide = const_cast<LPWSTR>(tmp.c_str());

    // Set up the queryset restrictions
    WSAQUERYSET wsaQuery{
        .dwSize = sizeof(WSAQUERYSET),
        .lpServiceClassId = const_cast<LPGUID>(&RFCOMM_PROTOCOL_UUID),
        .dwNameSpace = NS_BTH,
        .lpszContext = addrWide,
        .dwNumberOfCsAddrs = 0
    };

    // Start the lookup
    HANDLE hLookup{};
    DWORD flags = LUP_RETURN_ADDR;
    if (WSALookupServiceBegin(&wsaQuery, flags, &hLookup) == SOCKET_ERROR) return 0;

    // Continue the lookup
    DWORD size = 4096;
    LPWSAQUERYSET results = reinterpret_cast<LPWSAQUERYSET>(new char[size]);
    results->dwSize = size;
    results->dwNameSpace = NS_BTH;

    // Get the channel
    if (WSALookupServiceNext(hLookup, flags, &size, results) == NO_ERROR)
        ret = static_cast<uint8_t>(reinterpret_cast<PSOCKADDR_BTH>(results->lpcsaBuffer->RemoteAddr.lpSockaddr)->port);

    // Free buffer
    delete[] results;

    // End the lookup
    WSALookupServiceEnd(hLookup);
#else
    bdaddr_t deviceAddr;
    str2ba(addr, &deviceAddr);

    // `RFCOMM_UUID` is a 16-bit UUID for the RFCOMM protocol.
    // To turn this into a 128-bit UUID:
    //   | The 16-bit Attribute UUID replaces the x’s in the following:
    //   | 0000xxxx - 0000 - 1000 - 8000 - 00805F9B34FB
    // https://stackoverflow.com/a/36212021
    uint8_t uuidInt[] = {
        0x00, 0x00,
        0x00, RFCOMM_UUID, // Equals `0x00, 0x03` after preprocessing
        0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB // The Bluetooth base UUID
    };

    // Create the UUID
    uuid_t serviceUUID;
    sdp_uuid128_create(&serviceUUID, uuidInt);

    // Search list
    sdp_list_t* searchList = sdp_list_append(nullptr, &serviceUUID);

    uint32_t range = 0x0000ffff;
    sdp_list_t* attridList = sdp_list_append(nullptr, &range);

    // Initialize SDP session
    // We can't directly pass BDADDR_ANY to sdp_connect() because a "taking the address of an rvalue" error is thrown.
    bdaddr_t addrAny = { { 0, 0, 0, 0, 0, 0 } };
    sdp_session_t* session = sdp_connect(&addrAny, &deviceAddr, SDP_RETRY_IF_BUSY);
    if (!session) return 0; // Failed to connect to SDP session

    // Start SDP service search
    sdp_list_t* responseList = nullptr;
    int err = sdp_service_search_attr_req(session, searchList, SDP_ATTR_REQ_RANGE, attridList, &responseList);
    if (err != NO_ERROR) {
        // Failed to initialize service search, free all lists and stop session
        sdp_list_free(responseList, 0);
        sdp_list_free(searchList, 0);
        sdp_list_free(attridList, 0);
        sdp_close(session);
        return 0;
    }

    // Iterate through each of the service records
    // Unfortunately, there is no simple way to interpret the results of an SDP search, so we will have to use
    // this large mass of loops and conditionals to get the singular value that we want (the channel number)
    // Adapted from https://people.csail.mit.edu/albert/bluez-intro/x604.html (Example 4-8).
    for (sdp_list_t* r = responseList; r; r = r->next) {
        sdp_record_t* rec = reinterpret_cast<sdp_record_t*>(r->data);
        sdp_list_t* protoList;

        // Get a list of the protocol sequences
        if (sdp_get_access_protos(rec, &protoList) == NO_ERROR) {
            // Iterate through each protocol sequence
            for (sdp_list_t* p = protoList; p; p = p->next) {
                // Iterate through each protocol list of the protocol sequence
                for (sdp_list_t* pds = reinterpret_cast<sdp_list_t*>(p->data); pds; pds = pds->next) {
                    int proto = 0; // Bluetooth protocol

                    // Check protocol attributes
                    for (sdp_data_t* d = reinterpret_cast<sdp_data_t*>(pds->data); d; d = d->next) {
                        if (d->dtd == SDP_UINT8) {
                            if (proto == RFCOMM_UUID) {
                                ret = d->val.int8; // Finally, get the channel
                                break;
                            }
                        } else {
                            proto = sdp_uuid_to_proto(&d->val.uuid);
                        }
                    }
                }
                sdp_list_free(reinterpret_cast<sdp_list_t*>(p->data), 0);
            }
            sdp_list_free(protoList, 0);
        }
        sdp_record_free(rec);
    }
    sdp_close(session);
#endif

    // Return result
    return ret;
}
