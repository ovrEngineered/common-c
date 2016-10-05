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

#include <cxa_assert.h>
#include <cxa_stateMachine.h>

#include <provisioning.h>
#include <wlan.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef struct
{
	cxa_network_wifiManager_cb_t cb_provisioningEnter;
	cxa_network_wifiManager_ssid_cb_t cb_associatingWithSsid;
	cxa_network_wifiManager_ssid_cb_t cb_associatedWithSsid;
	cxa_network_wifiManager_ssid_cb_t cb_lostAssociation;
	cxa_network_wifiManager_ssid_cb_t cb_associateWithSsidFailed;
	cxa_network_wifiManager_ssid_cb_t cb_microApEnter;
	void *userVarIn;
}listener_t;


// ******** local function prototypes ********
static void stateCb_provision_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_provision_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_associating_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_associating_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_associated_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_associated_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_microAp_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_microAp_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);

static int appEventHandler(int event, void *data);
static int provEventHandler(enum prov_event event, void *arg, int len);
static void notify_provisioning(void);
static void notify_associating(void);
static void notify_associated(void);
static void notify_lostAssociation(void);
static void notify_associateFailed(void);
static void notify_microAp(void);


// ********  local variable declarations *********
static cxa_array_t listeners;
static listener_t listeners_raw[CXA_NETWORK_WIFIMGR_MAX_NUM_LISTENERS];

static bool hasStarted = false;
static cxa_stateMachine_t stateMachine;

static cxa_logger_t logger;

static struct provisioning_config provCfg =
{
		.prov_mode = PROVISIONING_EZCONNECT,
		.provisioning_event_handler = provEventHandler
};

static bool isNetConfigSet = false;
static struct wlan_network netConfig;

static char microAp_ssid[33] = "";
static char microAp_passphrase[65] = "";


// ******** global function implementations ********
void cxa_network_wifiManager_init(void)
{
	cxa_logger_init(&logger, "wifiMgr");

	// setup our internal arrays
	cxa_array_initStd(&listeners, listeners_raw);

	// setup our state machine
	cxa_stateMachine_init(&stateMachine, "wifiMgr");
	cxa_stateMachine_addState(&stateMachine, CXA_NETWORK_WIFISTATE_STARTUP, "startup", NULL, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, CXA_NETWORK_WIFISTATE_PROVISIONING, "provision", stateCb_provision_enter, NULL, stateCb_provision_leave, NULL);
	cxa_stateMachine_addState(&stateMachine, CXA_NETWORK_WIFISTATE_ASSOCIATING, "associating", stateCb_associating_enter, NULL, stateCb_associating_leave, NULL);
	cxa_stateMachine_addState(&stateMachine, CXA_NETWORK_WIFISTATE_ASSOCIATED, "associated", stateCb_associated_enter, NULL, stateCb_associated_leave, NULL);
	cxa_stateMachine_addState(&stateMachine, CXA_NETWORK_WIFISTATE_MICROAP, "microAp", stateCb_microAp_enter, NULL, stateCb_microAp_leave, NULL);
	cxa_stateMachine_setInitialState(&stateMachine, CXA_NETWORK_WIFISTATE_STARTUP);

	// @TODO try to load our netConfig from PSM
}


void cxa_network_wifiManager_addListener(cxa_network_wifiManager_cb_t cb_provisioniningEnterIn,
										 cxa_network_wifiManager_ssid_cb_t cb_associatingWithSsidIn,
										 cxa_network_wifiManager_ssid_cb_t cb_associatedWithSsidIn,
										 cxa_network_wifiManager_ssid_cb_t cb_lostAssociationWithSsidIn,
										 cxa_network_wifiManager_ssid_cb_t cb_associateWithSsidFailedIn,
										 cxa_network_wifiManager_ssid_cb_t cb_microApEnterIn,
										 void *userVarIn)
{
	listener_t newListener =
	{
			.cb_provisioningEnter = cb_provisioniningEnterIn,
			.cb_associatingWithSsid = cb_associatingWithSsidIn,
			.cb_associatedWithSsid = cb_associatedWithSsidIn,
			.cb_lostAssociation = cb_lostAssociationWithSsidIn,
			.cb_associateWithSsidFailed = cb_associateWithSsidFailedIn,
			.cb_microApEnter = cb_microApEnterIn,
			.userVarIn = userVarIn
	};

	cxa_assert(cxa_array_append(&listeners, &newListener));
}


void cxa_network_wifiManager_start(void)
{
	if( hasStarted ) return;
	app_framework_start(appEventHandler);
	hasStarted = true;
}


cxa_network_wifiManager_state_t cxa_network_wifiManager_getState(void)
{
	return cxa_stateMachine_getCurrentState(&stateMachine);
}


bool cxa_network_wifiManager_restart(void)
{
	if( !hasStarted ) return false;

	cxa_stateMachine_transition(&stateMachine, isNetConfigSet ? CXA_NETWORK_WIFISTATE_ASSOCIATING : CXA_NETWORK_WIFISTATE_PROVISIONING);
}


bool cxa_network_wifiManager_enterProvision(void)
{
	if( !hasStarted ) return false;

	cxa_stateMachine_transition(&stateMachine, CXA_NETWORK_WIFISTATE_PROVISIONING);

	return true;
}


bool cxa_network_wifiManager_enterMicroAp(const char* ssidIn, const char* passphraseIn)
{
	if( !hasStarted ) return false;

	cxa_assert(ssidIn);
	size_t passphraseLen_bytes = (passphraseIn != NULL) ? strlen(passphraseIn) : 0;
	if( passphraseIn != NULL ) cxa_assert((8 < passphraseLen_bytes) && (passphraseLen_bytes <= 64));

	// save our SSID and passphrase
	strlcpy(microAp_ssid, ssidIn, sizeof(microAp_ssid));
	microAp_passphrase[0] = 0;
	if( passphraseIn != NULL ) strlcpy(microAp_passphrase, passphraseIn, sizeof(microAp_passphrase));

	cxa_stateMachine_transition(&stateMachine, CXA_NETWORK_WIFISTATE_MICROAP);

	return true;
}


// ******** local function implementations ********
static void stateCb_provision_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "starting provisioning");

	app_reset_saved_network();
	prov_ezconnect_start(&provCfg);

	notify_provisioning();
}


static void stateCb_provision_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	prov_ezconnect_finish();
}


static void stateCb_associating_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "associating with '%s'", netConfig.ssid);

	app_sta_start_by_network(&netConfig);

	notify_associating();
}

static void stateCb_associating_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	if( nextStateIdIn != CXA_NETWORK_WIFISTATE_ASSOCIATED ) app_sta_stop();
}


static void stateCb_associated_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	notify_associated();
}


static void stateCb_associated_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	if( nextStateIdIn != CXA_NETWORK_WIFISTATE_ASSOCIATING ) app_sta_stop();
	notify_lostAssociation();
}


static void stateCb_microAp_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "starting microAp with ssid '%s'", microAp_ssid);

	// _MUST_ reset saved network first...if no passphrase, must be NULL
	app_reset_saved_network();
	app_uap_start_with_dhcp(microAp_ssid, (strlen(microAp_passphrase) > 0) ? microAp_passphrase : NULL);

	notify_microAp();
}


static void stateCb_microAp_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	app_uap_stop();
}


static int appEventHandler(int event, void *data)
{
	switch( event )
	{
		case AF_EVT_WLAN_INIT_DONE:
		{
			cxa_logger_info(&logger, "wlan init complete");
			cxa_stateMachine_transition(&stateMachine, isNetConfigSet ? CXA_NETWORK_WIFISTATE_ASSOCIATING : CXA_NETWORK_WIFISTATE_PROVISIONING);
			break;
		}

		case AF_EVT_NORMAL_CONNECTING:
		{
			break;
		}

		case AF_EVT_NORMAL_CONNECTED:
		{
			char ipAddrStr[16];
			app_network_ip_get(ipAddrStr);
			cxa_logger_info(&logger, "associated - ip:%s", ipAddrStr);
			cxa_stateMachine_transition(&stateMachine, CXA_NETWORK_WIFISTATE_ASSOCIATED);
			break;
		}

		case AF_EVT_NORMAL_CONNECT_FAILED:
		{
			cxa_logger_warn(&logger, "association failed %d, retrying", (data != NULL) ? *((app_conn_failure_reason_t*)data) : -1);
			notify_associateFailed();
			// returning WM_SUCCESS tells the app framework to keep retrying
			break;
		}

		default:
		{
			cxa_logger_debug(&logger, "unhandled app event: %d", event);
			break;
		}
	}

	return WM_SUCCESS;
}


static int provEventHandler(enum prov_event event, void *arg, int len)
{
	switch( event )
	{
		case PROV_NETWORK_SET_CB:
			if( len != sizeof(netConfig) )
			{
				cxa_logger_warn(&logger, "invalid size for provisioned network");
				break;
			}

			// copy our new network config
			memcpy(&netConfig, arg, sizeof(netConfig));
			isNetConfigSet = true;
			cxa_logger_debug(&logger, "provisioned for '%s' with %d byte passphrase", netConfig.ssid, strlen(netConfig.security.psk));

			cxa_stateMachine_transition(&stateMachine, CXA_NETWORK_WIFISTATE_ASSOCIATING);
			break;

		default:
			cxa_logger_debug(&logger, "unhandled provisioning event: %d", event);
			break;
	}

	return WM_SUCCESS;
}

static void notify_provisioning(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_provisioningEnter != NULL ) currListener->cb_provisioningEnter(currListener->userVarIn);
	}
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


static void notify_lostAssociation(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_lostAssociation != NULL ) currListener->cb_lostAssociation(netConfig.ssid, currListener->userVarIn);
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


static void notify_microAp(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_microApEnter != NULL ) currListener->cb_microApEnter(netConfig.ssid, currListener->userVarIn);
	}
}

