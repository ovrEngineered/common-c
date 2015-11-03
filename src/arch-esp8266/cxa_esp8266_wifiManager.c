/**
 * Copyright 2015 opencxa.org
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
#include "cxa_esp8266_wifiManager.h"


// ******** includes ********
#include <string.h>
#include <stddef.h>
#include <user_interface.h>
#include <cxa_assert.h>
#include <cxa_array.h>
#include <cxa_stateMachine.h>
#include <cxa_timeDiff.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define DEFAULT_CFGMODE_SSID		"cxa_config"


// ******** local type definitions ********
typedef enum
{
	STATE_INIT,
	STATE_CONFIG_MODE,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_CONNECT_FAILED
}state_t;


typedef struct
{
	cxa_esp8266_wifiManager_configMode_cb_t cb_configModeEnter;
	cxa_esp8266_wifiManager_configMode_numConnStationsChanged_cb_t cb_numConnStationsChanged;
	cxa_esp8266_wifiManager_configMode_cb_t cb_configModeLeave;
	cxa_esp8266_wifiManager_ssid_cb_t cb_connectingToSsid;
	cxa_esp8266_wifiManager_ssid_cb_t cb_connectedToSsid;
	cxa_esp8266_wifiManager_ssid_cb_t cb_lostConnectionToSsid;
	cxa_esp8266_wifiManager_ssid_cb_t cb_connectToSsidFailed;
	void *userVarIn;
}listener_t;


// ******** local function prototypes ********
static void stateCb_configMode_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_configMode_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_configMode_leave(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connecting_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connected_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connected_leave(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_connectFailed_enter(cxa_stateMachine_t *const smIn, void *userVarIn);


// ********  local variable declarations *********
static struct softap_config configModeCfg;

static uint8_t targetNetworkIndex;
static cxa_array_t storedNetworks;
static struct station_config storedNetworks_raw[CXA_ESP8266_WIFIMGR_MAX_NUM_STORED_NETWORKS];
static struct station_config* lastAssociatedNetwork;

static cxa_array_t listeners;
static listener_t listeners_raw[CXA_ESP8266_WIFIMGR_MAX_NUM_LISTENERS];

static cxa_stateMachine_t stateMachine;
static cxa_timeDiff_t td_genPurpose;

static cxa_logger_t logger;

// variables for config mode
static uint8_t numPreviouslyConnectedStations = 0;


// ******** global function implementations ********
void cxa_esp8266_wifiManager_init(const char* configModeSsidIn, cxa_timeBase_t *const timeBaseIn)
{
	cxa_logger_init(&logger, "wifiMgr");

	// clear our local variables
	memset(&configModeCfg, 0, sizeof(configModeCfg));
	memset(storedNetworks_raw, 0, sizeof(storedNetworks_raw));
	memset(listeners_raw, 0, sizeof(listeners_raw));
	lastAssociatedNetwork = NULL;

	// setup our config mode configuration
	configModeCfg.authmode = AUTH_OPEN;
	configModeCfg.beacon_interval = 100;
	configModeCfg.channel = 11;
	configModeCfg.max_connection = 4;
	configModeCfg.password[0] = 0;
	if( configModeSsidIn != NULL ) cxa_assert(strlen(configModeSsidIn) <= sizeof(configModeCfg.ssid));
	strlcpy((char*)configModeCfg.ssid, ((configModeSsidIn != NULL) ? configModeSsidIn : DEFAULT_CFGMODE_SSID), sizeof(configModeCfg.ssid));
	configModeCfg.ssid_hidden = 0;
	configModeCfg.ssid_len = strlen((char*)configModeCfg.ssid);

	// setup our internal arrays
	cxa_array_initStd(&listeners, listeners_raw);
	cxa_array_initStd(&storedNetworks, storedNetworks_raw);

	// setup our internal timing mechanisms
	cxa_timeDiff_init(&td_genPurpose, timeBaseIn, true);

	// setup our state machine
	cxa_stateMachine_init(&stateMachine, "wifiMgr");
	cxa_stateMachine_addState(&stateMachine, STATE_INIT, "init", NULL, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_CONFIG_MODE, "cfgMode", stateCb_configMode_enter, stateCb_configMode_state, stateCb_configMode_leave, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_CONNECTING, "connecting", stateCb_connecting_enter, stateCb_connecting_state, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_CONNECTED, "connected", stateCb_connected_enter, stateCb_connected_state, stateCb_connected_leave, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_CONNECT_FAILED, "connectFailed", stateCb_connectFailed_enter, NULL, NULL, NULL);
	cxa_stateMachine_transition(&stateMachine, STATE_INIT);
	cxa_stateMachine_update(&stateMachine);
}


void cxa_esp8266_wifiManager_addStoredNetwork(const char* ssidIn, const char* passphrase)
{
	cxa_assert(ssidIn);
	cxa_assert(strlen(ssidIn) > 0);

	struct station_config newConfig;
	cxa_assert(strlen(ssidIn) <= sizeof(newConfig.ssid));
	if( passphrase != NULL ) cxa_assert(strlen(passphrase) <= sizeof(newConfig.password));

	strlcpy((char*)newConfig.ssid, ssidIn, sizeof(newConfig.ssid));
	strlcpy((char*)newConfig.password, ((passphrase != NULL) ? passphrase : ""), sizeof(newConfig.password));
	cxa_assert(cxa_array_append(&storedNetworks, (void*)&newConfig));
}


void cxa_esp8266_wifiManager_addListener(cxa_esp8266_wifiManager_configMode_cb_t cb_configModeEnter,
										 cxa_esp8266_wifiManager_configMode_numConnStationsChanged_cb_t cb_numConnStationsChanged,
										 cxa_esp8266_wifiManager_configMode_cb_t cb_configModeLeave,
										 cxa_esp8266_wifiManager_ssid_cb_t cb_connectingToSsid,
										 cxa_esp8266_wifiManager_ssid_cb_t cb_connectedToSsid,
										 cxa_esp8266_wifiManager_ssid_cb_t cb_lostConnectionToSsid,
										 cxa_esp8266_wifiManager_ssid_cb_t cb_connectToSsidFailed,
										 void *userVarIn)
{
	listener_t newListener;
	newListener.cb_configModeEnter = cb_configModeEnter;
	newListener.cb_numConnStationsChanged = cb_numConnStationsChanged;
	newListener.cb_configModeLeave = cb_configModeLeave;
	newListener.cb_connectingToSsid = cb_connectingToSsid;
	newListener.cb_connectedToSsid = cb_connectedToSsid;
	newListener.cb_lostConnectionToSsid = cb_lostConnectionToSsid;
	newListener.cb_connectToSsidFailed = cb_connectToSsidFailed;
	newListener.userVarIn = userVarIn;

	cxa_assert(cxa_array_append(&listeners, &newListener));
}


void cxa_esp8266_wifiManager_start(void)
{
	cxa_assert(cxa_stateMachine_getCurrentState(&stateMachine) == STATE_INIT);

	if( cxa_array_getSize_elems(&storedNetworks) > 0 )
	{
		targetNetworkIndex = 0;
		cxa_stateMachine_transition(&stateMachine, STATE_CONNECTING);
		return;
	}
	else
	{
		cxa_stateMachine_transition(&stateMachine, STATE_CONFIG_MODE);
		return;
	}
}


void cxa_esp8266_wifiManager_update(void)
{
	cxa_stateMachine_update(&stateMachine);
}


// ******** local function implementations ********
static void stateCb_configMode_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	wifi_set_opmode(SOFTAP_MODE);
	wifi_softap_set_config_current(&configModeCfg);
	wifi_softap_dhcps_start();
	cxa_logger_info(&logger, "configuration mode active");

	// notify our listeners
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_configModeEnter != NULL ) currListener->cb_configModeEnter(currListener->userVarIn);
	}
}


static void stateCb_configMode_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	//@TODO check regularly to see if any of our networks have come back into range
	// (if we have stored networks)

	uint8_t numConnStations = wifi_softap_get_station_num();
	if( numConnStations != numPreviouslyConnectedStations )
	{
		cxa_logger_debug(&logger, (numConnStations > numPreviouslyConnectedStations) ? "new station connected" : "station disconnected");

		// notify our listeners
		cxa_array_iterate(&listeners, currListener, listener_t)
		{
			if( currListener == NULL ) continue;
			if( currListener->cb_numConnStationsChanged != NULL ) currListener->cb_numConnStationsChanged(numConnStations, currListener->userVarIn);
		}
		numPreviouslyConnectedStations = numConnStations;
	}
}


static void stateCb_configMode_leave(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	wifi_softap_dhcps_stop();
	wifi_set_opmode(NULL_MODE);
	cxa_logger_info(&logger, "leaving configuration mode");

	// notify our listeners
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_configModeLeave != NULL ) currListener->cb_configModeLeave(currListener->userVarIn);
	}
}


static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	// get our config and validate
	struct station_config* targetConfig = cxa_array_get(&storedNetworks, targetNetworkIndex);
	if( targetConfig == NULL )
	{
		// no more network configurations...enter configuration mode...
		cxa_logger_info(&logger, "out of stored networks...entering configuration mode");
		cxa_stateMachine_transition(&stateMachine, STATE_CONFIG_MODE);
		return;
	}
	if( strlen((char*)targetConfig->ssid) == 0 )
	{
		cxa_logger_warn(&logger, "invalid stored network at index %d, skipping", targetNetworkIndex);
		targetNetworkIndex++;
		cxa_stateMachine_transition(&stateMachine, STATE_CONNECTING);
	}
	cxa_logger_info(&logger, "connecting to '%s'", targetConfig->ssid);

	wifi_set_opmode(STATION_MODE);
	wifi_station_set_auto_connect(true);
	wifi_station_set_reconnect_policy(true);
	wifi_station_set_config_current(targetConfig);
	wifi_station_connect();

	// notify our listeners
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_connectingToSsid != NULL ) currListener->cb_connectingToSsid((char*)targetConfig->ssid, currListener->userVarIn);
	}
}


static void stateCb_connecting_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	uint8_t connStatus = wifi_station_get_connect_status();
	switch( connStatus )
	{
		case STATION_IDLE:
			if( cxa_timeDiff_isElapsed_recurring_ms(&td_genPurpose, 1000) ) cxa_logger_warn(&logger, "should be connecting but reports idle");
			break;

		case STATION_CONNECTING:
			// this is what we _should_ be doing
			break;

		case STATION_WRONG_PASSWORD:
		case STATION_NO_AP_FOUND:
		case STATION_CONNECT_FAIL:
			cxa_logger_debug(&logger, "connect failure: %d", connStatus);
			targetNetworkIndex++;
			cxa_stateMachine_transition(&stateMachine, STATE_CONNECTING);
			return;
			break;

		case STATION_GOT_IP:
			cxa_stateMachine_transition(&stateMachine, STATE_CONNECTED);
			return;
			break;
	}
}


static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	struct station_config* targetConfig = cxa_array_get(&storedNetworks, targetNetworkIndex);
	lastAssociatedNetwork = targetConfig;
	cxa_logger_info(&logger, "connected to '%s'", targetConfig->ssid);

	// notify our listeners
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_connectedToSsid != NULL ) currListener->cb_connectedToSsid((char*)lastAssociatedNetwork->ssid, currListener->userVarIn);
	}
}


static void stateCb_connected_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	if( wifi_station_get_connect_status() != STATION_GOT_IP )
	{
		cxa_logger_info(&logger, "disconnected from '%s'", lastAssociatedNetwork->ssid);
		targetNetworkIndex = 0;
		cxa_stateMachine_transition(&stateMachine, STATE_CONNECTING);
	}
}


static void stateCb_connected_leave(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	// lost our connection...notify our listeners
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_lostConnectionToSsid != NULL ) currListener->cb_lostConnectionToSsid((char*)lastAssociatedNetwork->ssid, currListener->userVarIn);
	}
	lastAssociatedNetwork = NULL;
}


static void stateCb_connectFailed_enter(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	cxa_logger_warn(&logger, "failed to connect to any known network, falling back to config mode");
	cxa_stateMachine_transition(&stateMachine, STATE_CONFIG_MODE);
}
