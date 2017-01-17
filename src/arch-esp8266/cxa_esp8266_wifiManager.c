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

#include "espressif/esp_common.h"

#include <cxa_assert.h>
#include <cxa_stateMachine.h>

#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef enum
{
	STATE_INIT,
	STATE_ASSOCIATING,
	STATE_ASSOCIATED,
}state_t;


typedef struct
{
	cxa_esp8266_wifiManager_ssid_cb_t cb_associatingWithSsid;
	cxa_esp8266_wifiManager_ssid_cb_t cb_associatedWithSsid;
	cxa_esp8266_wifiManager_ssid_cb_t cb_lostAssociationWithSsid;
	cxa_esp8266_wifiManager_ssid_cb_t cb_associateWithSsidFailed;
	void *userVarIn;
}listener_t;


// ******** local function prototypes ********
static void stateCb_associating_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_associating_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_associated_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn);
static void stateCb_associated_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_associated_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);


// ********  local variable declarations *********
static struct sdk_station_config storedNetwork;

static cxa_array_t listeners;
static listener_t listeners_raw[CXA_ESP8266_WIFIMGR_MAXNUM_LISTENERS];

static cxa_stateMachine_t stateMachine;
static cxa_timeDiff_t td_genPurpose;

static cxa_logger_t logger;


// ******** global function implementations ********
void cxa_esp8266_wifiManager_init(const char* ssidIn, const char* passphraseIn)
{
	cxa_assert(ssidIn);
	size_t ssidLen_bytes = strlen(ssidIn);
	cxa_assert( (0 < ssidLen_bytes) && (ssidLen_bytes <= 32) );

	cxa_logger_init(&logger, "wifiMgr");

	// setup our internal arrays
	cxa_array_initStd(&listeners, listeners_raw);

	// setup our internal timing mechanisms
	cxa_timeDiff_init(&td_genPurpose, true);

	// setup our stored network
	memset(&storedNetwork, 0, sizeof(storedNetwork));
	size_t passphraseLen_bytes = strlen(passphraseIn);
	cxa_assert(ssidLen_bytes <= sizeof(storedNetwork.ssid));
	if( passphraseIn != NULL ) cxa_assert(passphraseLen_bytes <= sizeof(storedNetwork.password));

	memcpy((char*)storedNetwork.ssid, ssidIn, ssidLen_bytes);
	memcpy((char*)storedNetwork.password, passphraseIn, passphraseLen_bytes);

	// setup our state machine
	cxa_stateMachine_init(&stateMachine, "wifiMgr");
	cxa_stateMachine_addState(&stateMachine, STATE_INIT, "init", NULL, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_ASSOCIATING, "associating", stateCb_associating_enter, stateCb_associating_state, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_ASSOCIATED, "associated", stateCb_associated_enter, stateCb_associated_state, stateCb_associated_leave, NULL);
	cxa_stateMachine_setInitialState(&stateMachine, STATE_INIT);
}


void cxa_esp8266_wifiManager_addListener(cxa_esp8266_wifiManager_ssid_cb_t cb_associatingWithSsid,
										 cxa_esp8266_wifiManager_ssid_cb_t cb_associatedWithSsid,
										 cxa_esp8266_wifiManager_ssid_cb_t cb_lostAssociationWithSsid,
										 cxa_esp8266_wifiManager_ssid_cb_t cb_associateWithSsidFailed,
										 void *userVarIn)
{
	listener_t newListener;
	newListener.cb_associatingWithSsid = cb_associatingWithSsid;
	newListener.cb_associatedWithSsid = cb_associatedWithSsid;
	newListener.cb_lostAssociationWithSsid = cb_lostAssociationWithSsid;
	newListener.cb_associateWithSsidFailed = cb_associateWithSsidFailed;
	newListener.userVarIn = userVarIn;

	cxa_assert(cxa_array_append(&listeners, &newListener));
}


bool cxa_esp8266_wifiManager_isAssociated(void)
{
	return (cxa_stateMachine_getCurrentState(&stateMachine) == STATE_ASSOCIATED);
}


void cxa_esp8266_wifiManager_start(void)
{
	cxa_assert(cxa_stateMachine_getCurrentState(&stateMachine) == STATE_INIT);

	cxa_stateMachine_transition(&stateMachine, STATE_ASSOCIATING);
}


void cxa_esp8266_wifiManager_update(void)
{
	cxa_stateMachine_update(&stateMachine);
}


// ******** local function implementations ********
static void stateCb_associating_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	sdk_wifi_set_opmode(STATION_MODE);
	sdk_wifi_station_set_auto_connect(true);
	sdk_wifi_station_set_config(&storedNetwork);
	sdk_wifi_station_connect();

	// notify our listeners
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_associatingWithSsid != NULL ) currListener->cb_associatingWithSsid((char*)storedNetwork.ssid, currListener->userVarIn);
	}
}


static void stateCb_associating_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	uint8_t connStatus = sdk_wifi_station_get_connect_status();
	switch( connStatus )
	{
		case STATION_IDLE:
			if( cxa_timeDiff_isElapsed_recurring_ms(&td_genPurpose, 1000) ) cxa_logger_warn(&logger, "should be associating but reports idle");
			break;

		case STATION_CONNECTING:
			// this is what we _should_ be doing
			break;

		case STATION_WRONG_PASSWORD:
		case STATION_NO_AP_FOUND:
		case STATION_CONNECT_FAIL:
			cxa_logger_debug(&logger, "associate failure: %d", connStatus);
			cxa_stateMachine_transition(&stateMachine, STATE_ASSOCIATING);
			return;
			break;

		case STATION_GOT_IP:
			cxa_stateMachine_transition(&stateMachine, STATE_ASSOCIATED);
			return;
			break;
	}
}


static void stateCb_associated_enter(cxa_stateMachine_t *const smIn, int prevStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "associated with '%s'", storedNetwork.ssid);

	// notify our listeners
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_associatedWithSsid != NULL ) currListener->cb_associatedWithSsid((char*)storedNetwork.ssid, currListener->userVarIn);
	}
}


static void stateCb_associated_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	if( sdk_wifi_station_get_connect_status() != STATION_GOT_IP )
	{
		cxa_logger_info(&logger, "disassociated from '%s'", storedNetwork.ssid);
		cxa_stateMachine_transition(&stateMachine, STATE_ASSOCIATING);
	}
}


static void stateCb_associated_leave(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	// lost our connection...notify our listeners
	cxa_array_iterate(&listeners, currListener, listener_t)
	{
		if( currListener == NULL ) continue;
		if( currListener->cb_lostAssociationWithSsid != NULL ) currListener->cb_lostAssociationWithSsid((char*)storedNetwork.ssid, currListener->userVarIn);
	}
}
