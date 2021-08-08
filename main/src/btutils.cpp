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

#ifdef _WIN32
#include <WinSock2.h>
#include <bluetoothapis.h>

#include "coreutils.hpp"

// Link with Bluetooth lib
#pragma comment(lib, "Bthprops.lib")
#else
#include <cerrno> // ENODEV

#include <gdbus/gdbus.h>

#define SOCKET_ERROR -1
#define NO_ERROR 0

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

static void connectHandler(DBusConnection*, void*) {
    // Once connected, notify the rest of the app by setting this flag
    BTUtils::glibConnected = true;
}

static void disconnectHandler(DBusConnection*, void*) {
    // Once disconnected, free all lists and allocated resources
    g_list_free(ctrlList);
    ctrlList = nullptr;
    defaultCtrl = nullptr;
    BTUtils::glibConnected = false;
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

void BTUtils::glibMainContextIteration() {
#ifndef _WIN32
    // The below line of code is used to poll GLib events without using its main loop facilities.
    // https://stackoverflow.com/a/64580893 (See comments)
    g_main_context_iteration(g_main_context_default(), false);
#endif
}

int BTUtils::getPaired(std::vector<DeviceData>& deviceList) {
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
    BLUETOOTH_DEVICE_INFO btDeviceInfo{ .dwSize = sizeof(btDeviceInfo) };
    HBLUETOOTH_DEVICE_FIND foundDevice = BluetoothFindFirstDevice(&searchCriteria, &btDeviceInfo);

    if (!foundDevice) return SOCKET_ERROR;

    // Loop through each found device
    do {
        // Device MAC address broken into six octets
        BYTE* addr = btDeviceInfo.Address.rgBytes;

        // Format address array into MAC address
        constexpr size_t len = 18;
        char mac[len];
        std::snprintf(mac, len, "%02X:%02X:%02X:%02X:%02X:%02X", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

        // Push back
        deviceList.push_back({ Bluetooth, fromWide(btDeviceInfo.szName), mac, 0, btDeviceInfo.Address.ullLong });
    } while (BluetoothFindNextDevice(foundDevice, &btDeviceInfo));
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
        deviceList.push_back({ Bluetooth, name, address, 0, 0 });
    }
#endif

    // The enumeration of devices was successful
    return NO_ERROR;
}
