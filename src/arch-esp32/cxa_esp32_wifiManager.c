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

#include "esp_event_loop.h"
#include "esp_smartConfig.h"
#include "esp_wifi.h"

#include <cxa_assert.h>
#include <cxa_console.h>
#include <cxa_stateMachine.h>


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
static bool isStaConfigSet(void);

static void stateCb_startup_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_provision_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_provision_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_associating_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_associating_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_associated_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_associated_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_microAp_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_microAp_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_restarting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);

static esp_err_t espCb_eventHandler(void *ctx, system_event_t *event);
static void espCb_smartConfig(smartconfig_status_t status, void *pdata);

static void notify_provisioning(void);
static void notify_associating(void);
static void notify_associated(void);
static void notify_lostAssociation(void);
static void notify_associateFailed(void);
static void notify_microAp(void);

static void consoleCb_setMode(cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_start(cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_stop(cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_smartCfg_start(cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_smartCfg_stop(cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_connect(cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_disconnect(cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_restore(cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_getCfg(cxa_ioStream_t *const ioStreamIn, void* userVarIn);


// ********  local variable declarations *********
static cxa_array_t listeners;
static listener_t listeners_raw[CXA_NETWORK_WIFIMGR_MAX_NUM_LISTENERS];

static cxa_stateMachine_t stateMachine;

static cxa_logger_t logger;

//static char microAp_ssid[33] = "";
//static char microAp_passphrase[65] = "";


// ******** global function implementations ********
void cxa_network_wifiManager_init(void)
{
	cxa_logger_init(&logger, "wifiMgr");

	// setup our internal arrays
	cxa_array_initStd(&listeners, listeners_raw);

	// setup our wifi sub-system
	tcpip_adapter_init();
	ESP_ERROR_CHECK( esp_event_loop_init(espCb_eventHandler, NULL) );
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_FLASH) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );

	// setup our state machine
	cxa_stateMachine_init(&stateMachine, "wifiMgr");
	cxa_stateMachine_addState(&stateMachine, CXA_NETWORK_WIFISTATE_IDLE, "idle", NULL, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, CXA_NETWORK_WIFISTATE_STARTUP, "startup", stateCb_startup_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, CXA_NETWORK_WIFISTATE_PROVISIONING, "provision", stateCb_provision_enter, NULL, stateCb_provision_leave, NULL);
	cxa_stateMachine_addState(&stateMachine, CXA_NETWORK_WIFISTATE_ASSOCIATING, "associating", stateCb_associating_enter, NULL, stateCb_associating_leave, NULL);
	cxa_stateMachine_addState(&stateMachine, CXA_NETWORK_WIFISTATE_ASSOCIATED, "associated", stateCb_associated_enter, NULL, stateCb_associated_leave, NULL);
	cxa_stateMachine_addState(&stateMachine, CXA_NETWORK_WIFISTATE_MICROAP, "microAp", stateCb_microAp_enter, NULL, stateCb_microAp_leave, NULL);
	cxa_stateMachine_addState(&stateMachine, CXA_NETWORK_WIFISTATE_RESTARTING, "restarting", stateCb_restarting_enter, NULL, NULL, NULL);
	cxa_stateMachine_setInitialState(&stateMachine, CXA_NETWORK_WIFISTATE_IDLE);


	cxa_console_addCommand("setMode", consoleCb_setMode, NULL);
	cxa_console_addCommand("start", consoleCb_start, NULL);
	cxa_console_addCommand("stop", consoleCb_stop, NULL);
	cxa_console_addCommand("sCfg_start", consoleCb_smartCfg_start, NULL);
	cxa_console_addCommand("sCfg_stop", consoleCb_smartCfg_stop, NULL);
	cxa_console_addCommand("conn", consoleCb_connect, NULL);
	cxa_console_addCommand("disconn", consoleCb_disconnect, NULL);
	cxa_console_addCommand("restore", consoleCb_restore, NULL);
	cxa_console_addCommand("getCfg", consoleCb_getCfg, NULL);
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
	if( cxa_stateMachine_getCurrentState(&stateMachine) != CXA_NETWORK_WIFISTATE_IDLE ) return;

	cxa_stateMachine_transition(&stateMachine, CXA_NETWORK_WIFISTATE_STARTUP);
}


cxa_network_wifiManager_state_t cxa_network_wifiManager_getState(void)
{
	return cxa_stateMachine_getCurrentState(&stateMachine);
}


void cxa_network_wifiManager_restart(void)
{
	if( cxa_stateMachine_getCurrentState(&stateMachine) == CXA_NETWORK_WIFISTATE_IDLE ) return;

	cxa_stateMachine_transition(&stateMachine, CXA_NETWORK_WIFISTATE_RESTARTING);
}


bool cxa_network_wifiManager_enterProvision(void)
{
	if( cxa_stateMachine_getCurrentState(&stateMachine) == CXA_NETWORK_WIFISTATE_IDLE ) return false;

	cxa_stateMachine_transition(&stateMachine, CXA_NETWORK_WIFISTATE_PROVISIONING);

	return true;
}


bool cxa_network_wifiManager_enterMicroAp(const char* ssidIn, const char* passphraseIn)
{
	if( cxa_stateMachine_getCurrentState(&stateMachine) == CXA_NETWORK_WIFISTATE_IDLE ) return false;

	cxa_assert(ssidIn);
	size_t passphraseLen_bytes = (passphraseIn != NULL) ? strlen(passphraseIn) : 0;
	if( passphraseIn != NULL ) cxa_assert((8 < passphraseLen_bytes) && (passphraseLen_bytes <= 64));

	cxa_stateMachine_transition(&stateMachine, CXA_NETWORK_WIFISTATE_MICROAP);

	return true;
}


// ******** local function implementations ********
static bool isStaConfigSet(void)
{
	wifi_config_t wifiConfig;
	esp_wifi_get_config(WIFI_IF_STA, &wifiConfig);

	cxa_logger_trace(&logger, "iscs '%s'", wifiConfig.sta.ssid);

	return (strlen(wifiConfig.sta.ssid) > 0);
}


static void stateCb_startup_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "starting wifi services");

	esp_wifi_start();
}


static void stateCb_provision_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "starting provisioning");

	esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
	esp_smartconfig_start(espCb_smartConfig);

	notify_provisioning();
}


static void stateCb_provision_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	esp_smartconfig_stop();
}


static void stateCb_associating_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	wifi_config_t cfg;
	esp_wifi_get_config(WIFI_IF_STA, &cfg);
	cxa_logger_info(&logger, "associating with '%s' '%s'", cfg.sta.ssid, cfg.sta.password);

	esp_wifi_connect();

	notify_associating();
}

static void stateCb_associating_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
//	if( nextStateIdIn != CXA_NETWORK_WIFISTATE_ASSOCIATED ) app_sta_stop();
}


static void stateCb_associated_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	notify_associated();
}


static void stateCb_associated_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
//	if( nextStateIdIn != CXA_NETWORK_WIFISTATE_ASSOCIATING ) app_sta_stop();
	notify_lostAssociation();
}


static void stateCb_microAp_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	wifi_config_t cfg;
	esp_wifi_get_config(WIFI_IF_STA, &cfg);
	cxa_logger_info(&logger, "starting microAp with ssid '%s'", cfg.sta.ssid);

	notify_microAp();
}


static void stateCb_microAp_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
}


static void stateCb_restarting_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	esp_wifi_stop();
}


static void espCb_smartConfig(smartconfig_status_t status, void *pdata)
{
	cxa_logger_trace(&logger, "smartConfig: %d", status);

	switch( status )
	{
		case SC_STATUS_LINK:
		{
			// make a local copy
			wifi_config_t cfg;
			memcpy(&cfg.sta, pdata, sizeof(cfg.sta));

			// save our config to persistent storage
			esp_wifi_set_config(WIFI_IF_STA, &cfg);

			// restart to apply
			cxa_network_wifiManager_restart();

			break;
		}

		default:
			break;
	}
}


static esp_err_t espCb_eventHandler(void *ctx, system_event_t *event)
{
	cxa_logger_trace(&logger, "evHandler: %d", event->event_id);

	switch( event->event_id )
	{
		case SYSTEM_EVENT_STA_START:
			if( cxa_stateMachine_getCurrentState(&stateMachine) == CXA_NETWORK_WIFISTATE_STARTUP )
			{
				// we're starting up, decide whether to provision or try to associate
				cxa_stateMachine_transition(&stateMachine, isStaConfigSet() ? CXA_NETWORK_WIFISTATE_ASSOCIATING : CXA_NETWORK_WIFISTATE_PROVISIONING);
			}
			break;

		case SYSTEM_EVENT_STA_STOP:
			if( cxa_stateMachine_getCurrentState(&stateMachine) == CXA_NETWORK_WIFISTATE_RESTARTING )
			{
				// finished stopping...now start
				cxa_stateMachine_transition(&stateMachine, CXA_NETWORK_WIFISTATE_STARTUP);
			}
			break;

		case SYSTEM_EVENT_STA_DISCONNECTED:
			// keep retrying
			cxa_stateMachine_transition(&stateMachine, isStaConfigSet() ? CXA_NETWORK_WIFISTATE_ASSOCIATING : CXA_NETWORK_WIFISTATE_PROVISIONING);
			break;

		default:
			break;
	}

	return ESP_OK;
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
	wifi_config_t cfg;
	esp_wifi_get_config(WIFI_IF_STA, &cfg);

	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_associatingWithSsid != NULL ) currListener->cb_associatingWithSsid(cfg.sta.ssid, currListener->userVarIn);
	}
}


static void notify_associated(void)
{
	wifi_config_t cfg;
	esp_wifi_get_config(WIFI_IF_STA, &cfg);

	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_associatedWithSsid != NULL ) currListener->cb_associatedWithSsid(cfg.sta.ssid, currListener->userVarIn);
	}
}


static void notify_lostAssociation(void)
{
	wifi_config_t cfg;
	esp_wifi_get_config(WIFI_IF_STA, &cfg);

	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_lostAssociation != NULL ) currListener->cb_lostAssociation(cfg.sta.ssid, currListener->userVarIn);
	}
}


static void notify_associateFailed(void)
{
	wifi_config_t cfg;
	esp_wifi_get_config(WIFI_IF_STA, &cfg);

	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_associateWithSsidFailed != NULL ) currListener->cb_associateWithSsidFailed(cfg.sta.ssid, currListener->userVarIn);
	}
}


static void notify_microAp(void)
{
	wifi_config_t cfg;
	esp_wifi_get_config(WIFI_IF_STA, &cfg);

	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_microApEnter != NULL ) currListener->cb_microApEnter(cfg.sta.ssid, currListener->userVarIn);
	}
}


static void consoleCb_restore(cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	esp_wifi_restore();
}


static void consoleCb_getCfg(cxa_ioStream_t *const ioStreamIn, void* userVarIn)
{
	wifi_config_t cfg;
	esp_wifi_get_config(WIFI_IF_STA, &cfg);

	cxa_logger_trace(&logger, "'%s'  '%s'", cfg.sta.ssid, cfg.sta.password);
}
