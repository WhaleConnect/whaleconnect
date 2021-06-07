// Copyright 2021 the Network Socket Terminal contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _WIN32
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include <unistd.h>
#include <sys/types.h>
#endif

#include "searchbt.hpp"
#include "sockets.hpp"
#include "util.hpp"

#ifdef _WIN32
/// <summary>
/// Get the channel of a Bluetooth device.
/// </summary>
/// <param name="addr">The MAC address of the device</param>
/// <returns>The channel of the device</returns>
/// <remarks>
/// The device has to be advertising an SDP session for this program to retrieve its channel number.
/// </remarks>
static uint16_t getSDPChannel(const char* addr) {
	// Return value
	uint16_t ret = 0;

	// Size of UTF-8 wide string in UTF-16 encoding
	int stringSize = MultiByteToWideChar(CP_UTF8, 0, addr, -1, nullptr, 0);

	// Allocate char buffer to contain new string
	wchar_t* addrWide = new wchar_t[stringSize];

	// Convert the address string to UTF-16
	MultiByteToWideChar(CP_UTF8, 0, addr, -1, addrWide, stringSize);

	// Set up the queryset restrictions
	WSAQUERYSETW wsaQuery{
		.dwSize = sizeof(WSAQUERYSETW),
		.lpServiceClassId = const_cast<LPGUID>(&RFCOMM_PROTOCOL_UUID),
		.dwNameSpace = NS_BTH,
		.lpszContext = addrWide,
		.dwNumberOfCsAddrs = 0
	};

	// Start the lookup
	HANDLE hLookup{};
	DWORD flags = LUP_FLUSHCACHE | LUP_RETURN_ADDR;
	if (WSALookupServiceBeginW(&wsaQuery, flags, &hLookup) == SOCKET_ERROR) return 0;

	// Continue the lookup
	DWORD size = 4096;
	LPWSAQUERYSETW results = reinterpret_cast<LPWSAQUERYSETW>(new char[size]);
	results->dwSize = size;
	results->dwNameSpace = NS_BTH;

	// Get the channel
	while (WSALookupServiceNextW(hLookup, flags, &size, results) == NO_ERROR)
		ret = static_cast<uint16_t>(reinterpret_cast<PSOCKADDR_BTH>(results->lpcsaBuffer->RemoteAddr.lpSockaddr)->port);

	// Free buffers
	delete[] results;
	delete[] addrWide;

	// End the lookup
	WSALookupServiceEnd(hLookup);

	// Return result
	return ret;
}
#endif

std::pair<int, std::vector<DeviceData>> Sockets::searchBluetoothDevices() {
	// Return value
	std::vector<DeviceData> ret;

	// MAC address of detected device
	char addrStr[18] = "";

	// Search takes slightly longer so divide the search time value to compensate
	uint8_t searchLen = static_cast<uint8_t>(Settings::btSearchTime / 1.28);

#ifdef _WIN32
	BTH_QUERY_DEVICE qDev{ .length = searchLen }; // Timeout in seconds
	BLOB bthConfig{ .cbSize = sizeof(BTH_QUERY_DEVICE), .pBlobData = reinterpret_cast<PBYTE>(&qDev) };

	// Begin Winsock lookup
	//
	// (LP)WSAQUERYSET, WSALookupServiceBegin, and WSALookupServiceNext are deprecated (warning C4996)
	//
	// These need to be replaced with a W-appended version (e.g. WSAQUERYSETW) to remove the deprecation warnings.
	// The W stands for "wide character". This will make the string fields of the structures the LPWSTR datatype, which
	// is a wide UTF-16 encoded string.
	//
	// These wide strings require a conversion into a UTF-8 std::string.

	// Query set structure
	HANDLE hLookup{};
	DWORD flags = LUP_RETURN_ADDR | LUP_RETURN_NAME | LUP_CONTAINERS | LUP_FLUSHCACHE;
	WSAQUERYSETW wsaQuery{ .dwSize = sizeof(WSAQUERYSETW), .dwNameSpace = NS_BTH, .lpBlob = &bthConfig };
	if (WSALookupServiceBeginW(&wsaQuery, flags, &hLookup) == SOCKET_ERROR) return { WSAGetLastError(), ret };

	// Query set return structure
	DWORD size = 4096;
	LPWSAQUERYSETW results = reinterpret_cast<LPWSAQUERYSETW>(new char[size]);

	// Set structure fields
	results->dwNameSpace = NS_BTH;
	results->dwSize = sizeof(WSAQUERYSETW);
	results->lpBlob = &bthConfig;

	// Call Winsock lookup until no more devices are left
	while (WSALookupServiceNextW(hLookup, flags, &size, results) == NO_ERROR) {
		// Bluetooth socket of the current device that was found
		PSOCKADDR_BTH btSock = reinterpret_cast<PSOCKADDR_BTH>(results->lpcsaBuffer->RemoteAddr.lpSockaddr);

		// Get MAC address (this raw integer representation is needed for connecting)
		uint64_t mac = (static_cast<uint64_t>(GET_NAP(btSock->btAddr)) << 32)
			| static_cast<uint64_t>(GET_SAP(btSock->btAddr));

		// The MAC address in hex form looks like: 0x3C71BF70DA46
		// It contains six octets in hexadecimal: 3C 71 BF 70 DA 46
		// We want to extract these and place them into an array.
		uint8_t values[6];

		// To do this, we set up a loop to iterate six times:
		for (int i = 0; i < 6; i++) {
			// On each iteration, we will apply a bitmask with the bitwise and (&) operator to get each octet.
			//
			// The '&' operator can be thought of as multiplication, along with the min value of a hex digit (0x0) being
			// 0 and the max value (0xF) being 1.
			// Anything multiplied by 1 will equal itself, and anything multiplied by 0 will equal 0.
			// Likewise:
			// => &-ing any hex digit with 0xF will result in no change.
			// => &-ing any hex digit with 0x0 will result in 0.
			// => &-ing any hex digit with a value in between 0x0 and 0xF will change the value (like multiplying a
			//      number with a decimal), we don't want that here.
			//
			// Since each octet has two digits, we need a bitmask with two 0xF's, yielding 0xFF. To position the 0xFF
			// block to extract the two digits of one single octet, we bitshift it a number of times to the left. This
			// specific number is the loop index (int i) times 8. The process of bitshifting will create 0's to the
			// right of the FF block, and imaginary 0's do already exist to the left as well. When &-ed with the entire
			// address, the FF component will preserve that one octet and the 0's turn the rest of the address into 0's.
			//
			// mac & 00 00 00 00 00 FF (0xFF << 0)  = 00 00 00 00 00 46 = 0x46 (iteration 0)
			// mac & 00 00 00 00 FF 00 (0xFF << 8)  = 00 00 00 00 DA 00 = 0xDA00 (iteration 1)
			// mac & 00 00 00 FF 00 00 (0xFF << 16) = 00 00 00 70 00 00 = 0x700000 (iteration 2)
			// mac & 00 00 FF 00 00 00 (0xFF << 24) = 00 00 BF 00 00 00 = 0xBF000000 (iteration 3)
			// mac & 00 FF 00 00 00 00 (0xFF << 32) = 00 71 00 00 00 00 = 0x7100000000 (iteration 4)
			// mac & FF 00 00 00 00 00 (0xFF << 40) = 3C 00 00 00 00 00 = 0x3C0000000000 (iteration 5)
			//       3C 71 BF 70 DA 46
			//
			// Since the resultant values have trailing zeros, we will have to bitshift them to the right to remove the
			// zeros.
			// This whole approach will generate the octets backward, so the array index will have to iterate down via
			// subtraction.
			uint8_t bitshift = static_cast<uint8_t>(i * 8); // Bitshift value
			uint64_t bitmask = static_cast<uint64_t>(0xFF) << bitshift; // Bitmask to extract an octet
			values[5 - i] = static_cast<uint8_t>((mac & bitmask) >> bitshift); // Save the extracted octet
		}

		// Convert name from UTF-16 to UTF-8
		LPWSTR name = results->lpszServiceInstanceName;

		// Size of UTF-16 wide string in UTF-8 encoding
		int stringSize = WideCharToMultiByte(CP_UTF8, 0, name, -1, 0, 0, nullptr, nullptr);

		// Allocate char buffer to contain new string
		char* tmpBuf = new char[stringSize];

		// Convert the wide string to UTF-8 and store it in the buffer
		WideCharToMultiByte(CP_UTF8, 0, name, -1, tmpBuf, stringSize, nullptr, nullptr);

		// Format MAC address into string
		std::snprintf(addrStr, ARRAY_LEN(addrStr), "%02X:%02X:%02X:%02X:%02X:%02X", values[0], values[1], values[2],
			values[3], values[4], values[5]);

		// Add the device's data to the vector
		ret.push_back({ Bluetooth, tmpBuf, addrStr, getSDPChannel(addrStr), mac });

		// Free name buffer
		delete[] tmpBuf;
	}

	// Free results
	delete[] results;

	// Clean up resources
	WSALookupServiceEnd(hLookup);
#else
	constexpr size_t nameLen = 248; // Length of friendly name of Bluetooth device
	char name[nameLen] = ""; // Buffer to store name of device

	// Get Bluetooth device id
	int deviceId = hci_get_route(nullptr);

	// Open a Bluetooth socket to the local Bluetooth adapter, not a connection to a remote device as with socket()
	int sock = hci_open_dev(deviceId);

	// Check if any values are negative indicating a failure
	if (deviceId < 0 || sock < 0) return { errno, ret };

	// Bluetooth SDP UUID
	// "The UUID for the SPP Serial Port service is defined by the Bluetooth SIG to be 0x1101."
	// https://stackoverflow.com/a/4637749
	uuid_t serviceUUID;
	uint16_t serviceClass = 0x1101;
	sdp_uuid16_create(&serviceUUID, serviceClass);

	// Search list
	sdp_list_t* searchList = sdp_list_append(nullptr, &serviceUUID);

	uint32_t range = 0x0000ffff;
	sdp_list_t* attridList = sdp_list_append(nullptr, &range);

	// Search options
	int maxRsp = 255; // Return up to maxRsp devices
	int flags = IREQ_CACHE_FLUSH; // Flush out cache of previously detected devices
	inquiry_info* ii = reinterpret_cast<inquiry_info*>(new char[maxRsp * sizeof(inquiry_info)]);

	// Search for nearby devices
	int numRsp = hci_inquiry(deviceId, searchLen, maxRsp, nullptr, &ii, flags);
	if (numRsp < 0) return { errno, ret };

	// Iterate through each found device
	for (int i = 0; i < numRsp; i++) {
		// Get the current device
		inquiry_info* currentDevice = ii + i;
		bdaddr_t currentAddr = currentDevice->bdaddr;

		// Store its MAC address in the addr variable
		ba2str(&currentAddr, addrStr);

		// Clear out name buffer
		std::memset(name, 0, nameLen);

		// Get the device's name, if this returns a negative status code, use a placeholder name
		if (hci_read_remote_name(sock, &currentAddr, nameLen, name, 0) < 0) std::strncpy(name, "Unknown", nameLen);

		// All code below is for SDP (Service Discovery Protocol) to get the RFCOMM channel of the device found.
		uint16_t channel = 0;

		// Initialize SDP session
		// We can't directly pass BDADDR_ANY to sdp_connect() b/c GCC complains about taking the address of an rvalue.
		bdaddr_t addrAny = { { 0, 0, 0, 0, 0, 0} };
		sdp_session_t* session = sdp_connect(&addrAny, &currentAddr, SDP_RETRY_IF_BUSY);
		if (!session) continue; // Failed to connect to SDP session

		// Start SDP service search
		sdp_list_t* responseList = nullptr;
		int err = sdp_service_search_attr_req(session, searchList, SDP_ATTR_REQ_RANGE, attridList, &responseList);
		if (err != NO_ERROR) {
			// Failed to initialize service search, free all lists and stop session
			sdp_list_free(responseList, 0);
			sdp_list_free(searchList, 0);
			sdp_list_free(attridList, 0);
			sdp_close(session);
			continue;
		}

		// Iterate through each of the service records.
		// Unfortunately, there is no simple way to interpret the results of an SDP search, so we will have to use
		// this large mass of loops and conditionals to get the singular value that we want (the channel number)
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
								if (proto == RFCOMM_UUID) channel = d->val.int8; // Finally, get the channel
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

		// Add device information to vector
		ret.push_back({ Bluetooth, name, addrStr, channel, currentAddr });
	}

	// Clean up resources
	delete[] ii;

	// Close socket
	close(sock);
#endif

	// Return result
	return { NO_ERROR, ret };
}
