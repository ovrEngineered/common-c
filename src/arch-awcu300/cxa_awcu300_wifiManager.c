/**
 * Copyright 2016 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#include "cxa_network_wifiManager.h"


// ******** includes ********
#include <string.h>

#include <app_framework.h>
#include <wlan.h>

#include <cxa_assert.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef struct
{
	cxa_network_wifiManager_ssid_cb_t cb_associatingWithSsid;
	cxa_network_wifiManager_ssid_cb_t cb_associatedWithSsid;
	cxa_network_wifiManager_ssid_cb_t cb_associateWithSsidFailed;
	void *userVarIn;
}listener_t;


// ******** local function prototypes ********
static int appEventHandler(int event, void *data);
static void notify_associating(void);
static void notify_associated(void);
static void notify_associateFailed(void);


// ********  local variable declarations *********
static cxa_array_t listeners;
static listener_t listeners_raw[CXA_NETWORK_WIFIMGR_MAX_NUM_LISTENERS];

static cxa_logger_t logger;

static struct wlan_network netConfig = {
		.ip.ipv4.addr_type = ADDR_TYPE_DHCP,
		.name = "DXY",
		.security.type = WLAN_SECURITY_WPA2,
		.type = WLAN_BSS_ROLE_STA
};


// ******** global function implementations ********
void cxa_network_wifiManager_init(const char* ssidIn, const char* passphraseIn)
{
	cxa_assert(ssidIn);
	size_t ssidLen_bytes = strlen(ssidIn);
	cxa_assert( (0 < ssidLen_bytes) && (ssidLen_bytes <= 32) );

	cxa_logger_init(&logger, "wifiMgr");

	// setup our internal arrays
	cxa_array_initStd(&listeners, listeners_raw);

	// setup our stored network
	strlcpy(netConfig.ssid, ssidIn, sizeof(netConfig.ssid));
	strlcpy(netConfig.security.psk, passphraseIn, sizeof(netConfig.security.psk));
	netConfig.security.psk_len = strlen(passphraseIn);
}


void cxa_network_wifiManager_addListener(cxa_network_wifiManager_ssid_cb_t cb_associatingWithSsid,
										 cxa_network_wifiManager_ssid_cb_t cb_associatedWithSsid,
										 cxa_network_wifiManager_ssid_cb_t cb_associateWithSsidFailed,
										 void *userVarIn)
{
	listener_t newListener;
	newListener.cb_associatingWithSsid = cb_associatingWithSsid;
	newListener.cb_associatedWithSsid = cb_associatedWithSsid;
	newListener.cb_associateWithSsidFailed = cb_associateWithSsidFailed;
	newListener.userVarIn = userVarIn;

	cxa_assert(cxa_array_append(&listeners, &newListener));
}


bool cxa_network_wifiManager_isAssociated(void)
{
	app_conn_status_t currState;
	app_conn_failure_reason_t reason;
	int numAttempts;
	app_get_connection_status(&currState, &reason, &numAttempts);

	return (currState == CONN_STATE_CONNECTED);
}


void cxa_network_wifiManager_start(void)
{
	app_framework_start(appEventHandler);
}


// ******** local function implementations ********
static int appEventHandler(int event, void *data)
{
	switch( event )
	{
		case AF_EVT_WLAN_INIT_DONE:
		{
			cxa_logger_info(&logger, "wlan init complete");
			app_sta_start_by_network(&netConfig);
			break;
		}

		case AF_EVT_NORMAL_CONNECTING:
		{
			cxa_logger_info(&logger, "associating with '%s'", netConfig.ssid);
			notify_associating();
			break;
		}

		case AF_EVT_NORMAL_CONNECTED:
		{
			char ipAddrStr[16];
			app_network_ip_get(ipAddrStr);
			cxa_logger_info(&logger, "associated - ip:%s", ipAddrStr);
			notify_associated();
			break;
		}

		case AF_EVT_NORMAL_CONNECT_FAILED:
			cxa_logger_warn(&logger, "association failed");
			notify_associateFailed();
			break;

		default:
			cxa_logger_debug(&logger, "unhandled event: %d", event);
			break;
	}

	return 0;
}


static void notify_associating(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_associatingWithSsid != NULL ) currListener->cb_associatingWithSsid(netConfig.ssid, currListener->userVarIn);
	}
}


static void notify_associated(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_associatedWithSsid != NULL ) currListener->cb_associatedWithSsid(netConfig.ssid, currListener->userVarIn);
	}
}


static void notify_associateFailed(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_associateWithSsidFailed != NULL ) currListener->cb_associateWithSsidFailed(netConfig.ssid, currListener->userVarIn);
	}
}


