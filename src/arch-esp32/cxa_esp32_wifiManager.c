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
#define CONNECTION_TIMEOUT_MS				10000


// ******** local type definitions ********
typedef enum
{
	STATE_IDLE,
	STATE_STARTUP,
	STATE_ASSOCIATING,
	STATE_ASSOCIATED,
	STATE_CONNECTED,
	STATE_CONNECTION_TIMEOUT,
	STATE_STA_STOPPING,
	STATE_PROVISIONING,
	STATE_MICROAP,
	STATE_MICROAP_STOPPING
}internalState_t;


typedef enum
{
	INTTAR_MODE_DISABLED,
	INTTAR_MODE_MICROAP,
	INTTAR_MODE_NORMAL,
	INTTAR_MODE_PROVISIONING
}internalTargetMode_t;


typedef struct
{
	cxa_network_wifiManager_cb_t cb_idleEnter;
	cxa_network_wifiManager_cb_t cb_provisioningEnter;
	cxa_network_wifiManager_ssid_cb_t cb_connectingToSsid;
	cxa_network_wifiManager_ssid_cb_t cb_connectedToSsid;
	cxa_network_wifiManager_cb_t cb_disconnected;
	cxa_network_wifiManager_ssid_cb_t cb_connectionToSsidFailed;
	cxa_network_wifiManager_ssid_cb_t cb_microApEnter;
	void *userVarIn;
}listener_t;


// ******** local function prototypes ********
static bool isStaConfigSet(void);

static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_startup_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_associating_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_connected_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_connectionTimeout_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_staStopping_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_provisioning_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_microAp_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_microApStopping_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);

static esp_err_t espCb_eventHandler(void *ctx, system_event_t *event);
static void espCb_smartConfig(smartconfig_status_t status, void *pdata);

static void notify_idle(void);
static void notify_provisioning(void);
static void notify_connecting(void);
static void notify_connected(void);
static void notify_disconnected(void);
static void notify_connectFailed(void);
static void notify_microAp(void);

static void consoleCb_restore(cxa_ioStream_t *const ioStreamIn, void* userVarIn);
static void consoleCb_getCfg(cxa_ioStream_t *const ioStreamIn, void* userVarIn);


// ********  local variable declarations *********
static cxa_array_t listeners;
static listener_t listeners_raw[CXA_NETWORK_WIFIMGR_MAX_NUM_LISTENERS];

static internalTargetMode_t targetWifiMode = INTTAR_MODE_DISABLED;
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
	cxa_stateMachine_addState(&stateMachine, STATE_IDLE, "idle", stateCb_idle_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_STARTUP, "startup", stateCb_startup_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState_timed(&stateMachine, STATE_ASSOCIATING, "associating", STATE_CONNECTION_TIMEOUT, CONNECTION_TIMEOUT_MS, stateCb_associating_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState_timed(&stateMachine, STATE_ASSOCIATED, "associated", STATE_CONNECTION_TIMEOUT, CONNECTION_TIMEOUT_MS, NULL, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_CONNECTED, "connected", stateCb_connected_enter, NULL, stateCb_connected_leave, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_CONNECTION_TIMEOUT, "connTimout", stateCb_connectionTimeout_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_STA_STOPPING, "staStopping", stateCb_staStopping_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_PROVISIONING, "provisioning", stateCb_provisioning_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_MICROAP, "microAp", stateCb_microAp_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_MICROAP_STOPPING, "microApStopping", stateCb_microApStopping_enter, NULL, NULL, NULL);
	cxa_stateMachine_setInitialState(&stateMachine, STATE_IDLE);

	cxa_console_addCommand("restore", consoleCb_restore, NULL);
	cxa_console_addCommand("getCfg", consoleCb_getCfg, NULL);
}


void cxa_network_wifiManager_addListener(cxa_network_wifiManager_cb_t cb_idleEnterIn,
										 cxa_network_wifiManager_cb_t cb_provisioningEnterIn,
										 cxa_network_wifiManager_ssid_cb_t cb_connectingToSsidIn,
										 cxa_network_wifiManager_ssid_cb_t cb_connectedToSsidIn,
										 cxa_network_wifiManager_cb_t cb_disconnectedIn,
										 cxa_network_wifiManager_ssid_cb_t cb_connectionToSsidFailedIn,
										 cxa_network_wifiManager_ssid_cb_t cb_microApEnterIn,
										 void *userVarIn)
{
	listener_t newListener =
	{
			.cb_idleEnter = cb_idleEnterIn,
			.cb_provisioningEnter = cb_provisioningEnterIn,
			.cb_connectingToSsid = cb_connectingToSsidIn,
			.cb_connectedToSsid = cb_connectedToSsidIn,
			.cb_disconnected = cb_disconnectedIn,
			.cb_connectionToSsidFailed = cb_connectionToSsidFailedIn,
			.cb_microApEnter = cb_microApEnterIn,
			.userVarIn = userVarIn
	};

	cxa_assert(cxa_array_append(&listeners, &newListener));
}


void cxa_network_wifiManager_startNormal(void)
{
	if( cxa_stateMachine_getCurrentState(&stateMachine) != STATE_IDLE ) return;

	targetWifiMode = INTTAR_MODE_NORMAL;

	cxa_stateMachine_transition(&stateMachine, STATE_STARTUP);
}


void cxa_network_wifiManager_startMicroAp(void)
{
	if( cxa_stateMachine_getCurrentState(&stateMachine) != STATE_IDLE ) return;

	targetWifiMode = INTTAR_MODE_MICROAP;

	cxa_stateMachine_transition(&stateMachine, STATE_STARTUP);
}


void cxa_network_wifiManager_stop(void)
{
	if( cxa_stateMachine_getCurrentState(&stateMachine) == STATE_IDLE ) return;

	targetWifiMode = INTTAR_MODE_DISABLED;

	cxa_stateMachine_transition(&stateMachine, (targetWifiMode == INTTAR_MODE_MICROAP) ? STATE_MICROAP_STOPPING : STATE_STA_STOPPING);
}


cxa_network_wifiManager_state_t cxa_network_wifiManager_getState(void)
{
	cxa_network_wifiManager_state_t retVal = CXA_NETWORK_WIFISTATE_IDLE;

	switch( cxa_stateMachine_getCurrentState(&stateMachine) )
	{
		case STATE_IDLE:
		case STATE_STARTUP:
		case STATE_STA_STOPPING:
		case STATE_MICROAP_STOPPING:
			retVal = CXA_NETWORK_WIFISTATE_IDLE;
			break;

		case STATE_ASSOCIATING:
		case STATE_ASSOCIATED:
		case STATE_CONNECTION_TIMEOUT:
			retVal = CXA_NETWORK_WIFISTATE_CONNECTING;
			break;

		case STATE_CONNECTED:
			retVal = CXA_NETWORK_WIFISTATE_CONNECTED;
			break;

		case STATE_PROVISIONING:
			retVal = CXA_NETWORK_WIFISTATE_PROVISIONING;
			break;

		case STATE_MICROAP:
			retVal = CXA_NETWORK_WIFISTATE_MICROAP;
			break;
	}
	return retVal;
}


void cxa_network_wifiManager_restart(void)
{
	if( cxa_stateMachine_getCurrentState(&stateMachine) == STATE_IDLE ) return;

	cxa_stateMachine_transition(&stateMachine, (targetWifiMode == INTTAR_MODE_MICROAP) ? STATE_MICROAP_STOPPING : STATE_STA_STOPPING);
}


bool cxa_network_wifiManager_enterProvision(void)
{
	if( cxa_stateMachine_getCurrentState(&stateMachine) == STATE_IDLE ) return false;

	targetWifiMode = INTTAR_MODE_PROVISIONING;

	cxa_stateMachine_transition(&stateMachine, (targetWifiMode == INTTAR_MODE_MICROAP) ? STATE_MICROAP_STOPPING : STATE_STA_STOPPING);

	return true;
}


bool cxa_network_wifiManager_enterMicroAp(const char* ssidIn, const char* passphraseIn)
{
	internalState_t currState = cxa_stateMachine_getCurrentState(&stateMachine);
	if( (currState == STATE_IDLE) ||
		(currState == STATE_MICROAP) ) return false;

	cxa_assert(ssidIn);
	size_t passphraseLen_bytes = (passphraseIn != NULL) ? strlen(passphraseIn) : 0;
	if( passphraseIn != NULL ) cxa_assert((8 < passphraseLen_bytes) && (passphraseLen_bytes <= 64));

	targetWifiMode = INTTAR_MODE_MICROAP;

	if( currState != STATE_MICROAP_STOPPING ) cxa_stateMachine_transition(&stateMachine, STATE_STA_STOPPING);

	return true;
}


// ******** local function implementations ********
static bool isStaConfigSet(void)
{
	wifi_config_t wifiConfig;
	esp_wifi_get_config(WIFI_IF_STA, &wifiConfig);

	return (strlen(wifiConfig.sta.ssid) > 0);
}


static void stateCb_idle_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "becoming idle");

	esp_wifi_stop();

	notify_idle();
}


static void stateCb_startup_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "starting wifi services");

	if( targetWifiMode == INTTAR_MODE_DISABLED )
	{
		cxa_stateMachine_transition(&stateMachine, STATE_IDLE);
		return;
	}

	esp_wifi_set_mode((targetWifiMode == INTTAR_MODE_MICROAP) ? WIFI_MODE_AP : WIFI_MODE_STA);
	esp_wifi_start();
}


static void stateCb_associating_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	wifi_config_t wifiConfig;
	esp_wifi_get_config(WIFI_IF_STA, &wifiConfig);
	cxa_logger_info(&logger, "associating to '%s'", wifiConfig.sta.ssid);

	esp_wifi_connect();

	notify_connecting();
}


static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	wifi_config_t wifiConfig;
	esp_wifi_get_config(WIFI_IF_STA, &wifiConfig);
	cxa_logger_info(&logger, "connected to '%s'", wifiConfig.sta.ssid);

	notify_connected();
}


static void stateCb_connected_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	wifi_config_t wifiConfig;
	esp_wifi_get_config(WIFI_IF_STA, &wifiConfig);
	cxa_logger_info(&logger, "disconnected from '%s'", wifiConfig.sta.ssid);

	notify_disconnected();
}


static void stateCb_connectionTimeout_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	wifi_config_t wifiConfig;
	esp_wifi_get_config(WIFI_IF_STA, &wifiConfig);
	cxa_logger_info(&logger, "connection to '%s' timed out...retrying", wifiConfig.sta.ssid);

	notify_connectFailed();

	cxa_stateMachine_transition(&stateMachine, STATE_STA_STOPPING);
}


static void stateCb_staStopping_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	esp_wifi_stop();
}


static void stateCb_provisioning_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "starting provisioning");

	esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
	esp_smartconfig_start(espCb_smartConfig);

	notify_provisioning();
}


static void stateCb_microAp_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_warn(&logger, "not yet implemented");
}


static void stateCb_microApStopping_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_warn(&logger, "not yet implemented");
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

			cxa_logger_info(&logger, "provisioned for ssid:'%s'", cfg.sta.ssid);

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

	internalState_t currState = cxa_stateMachine_getCurrentState(&stateMachine);

	switch( event->event_id )
	{
		case SYSTEM_EVENT_STA_START:
			if( currState == STATE_STARTUP )
			{
				// we're starting up, decide whether to provision or try to associate
				cxa_stateMachine_transition(&stateMachine, isStaConfigSet() ? STATE_ASSOCIATING : STATE_PROVISIONING);
			}
			break;

		case SYSTEM_EVENT_STA_STOP:
			if( currState == STATE_STA_STOPPING )
			{
				// finished stopping...now see what we should do
				cxa_stateMachine_transition(&stateMachine, (targetWifiMode == INTTAR_MODE_DISABLED) ? STATE_IDLE : STATE_STARTUP);
			}
			else
			{
				cxa_logger_warn(&logger, "unexpected STA stop...restarting WiFi");
				cxa_network_wifiManager_restart();
			}
			break;

		case SYSTEM_EVENT_STA_DISCONNECTED:
			if( currState == STATE_ASSOCIATING )
			{
				// this means we immediately failed to associate...
				cxa_stateMachine_transition(&stateMachine, STATE_CONNECTION_TIMEOUT);
			}
			else
			{
				// keep retrying
				cxa_stateMachine_transition(&stateMachine, isStaConfigSet() ? STATE_ASSOCIATING : STATE_PROVISIONING);
			}
			break;

		case SYSTEM_EVENT_STA_GOT_IP:
			// we're associated!!
			cxa_stateMachine_transition(&stateMachine, STATE_CONNECTED);
			break;

		default:
			break;
	}

	return ESP_OK;
}


static void notify_idle(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_idleEnter != NULL ) currListener->cb_idleEnter(currListener->userVarIn);
	}
}


static void notify_provisioning(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_provisioningEnter != NULL ) currListener->cb_provisioningEnter(currListener->userVarIn);
	}
}


static void notify_connecting(void)
{
	wifi_config_t cfg;
	esp_wifi_get_config(WIFI_IF_STA, &cfg);

	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_connectingToSsid != NULL ) currListener->cb_connectingToSsid(cfg.sta.ssid, currListener->userVarIn);
	}
}


static void notify_connected(void)
{
	wifi_config_t cfg;
	esp_wifi_get_config(WIFI_IF_STA, &cfg);

	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_connectedToSsid != NULL ) currListener->cb_connectedToSsid(cfg.sta.ssid, currListener->userVarIn);
	}
}


static void notify_disconnected(void)
{
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_disconnected != NULL ) currListener->cb_disconnected(currListener->userVarIn);
	}
}


static void notify_connectFailed(void)
{
	wifi_config_t cfg;
	esp_wifi_get_config(WIFI_IF_STA, &cfg);

	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_connectionToSsidFailed != NULL ) currListener->cb_connectionToSsidFailed(cfg.sta.ssid, currListener->userVarIn);
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
