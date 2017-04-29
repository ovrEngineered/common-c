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
 */
#include <cxa_mqtt_connectionManager.h>


// ******** includes ********
#include <stdlib.h>
#include <string.h>
#include <cxa_assert.h>
#include <cxa_network_wifiManager.h>
#include <cxa_led.h>
#define CXA_LOG_LEVEL					CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>
#include <cxa_stateMachine.h>
#include <cxa_timeBase.h>
#include <cxa_uniqueId.h>

#include <cxa_config.h>


// ******** local macro definitions ********
#define BLINKPERIODMS_ON_CONFIG			100
#define BLINKPERIODMS_OFF_CONFIG		500

#define BLINKPERIODMS_ON_ASSOC			1000
#define BLINKPERIODMS_OFF_ASSOC			1000

#define BLINKPERIODMS_ON_CONNECTING		500
#define BLINKPERIODMS_OFF_CONNECTING	500

#define BLINKPERIODMS_ON_CONNECTED		5000
#define BLINKPERIODMS_OFF_CONNECTED		100

#define BLINKPERIODMS_ON_ERROR			100
#define BLINKPERIODMS_OFF_ERROR			100


// ******** local type definitions ********
typedef enum
{
	STATE_ASSOCIATING,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_CONNECT_STANDOFF,
	STATE_ERROR
}state_t;


// ******** local function prototypes ********
static void cxa_mqtt_connManager_commonInit(cxa_led_t *const ledConnIn,
											char *const hostNameIn, uint16_t portNumIn, bool useTlsIn,
											char *const usernameIn, uint8_t *const passwordIn, uint16_t passwordLen_bytesIn,
											const char* serverRootCertIn, size_t serverRootCertLen_bytesIn,
											const char* clientCertIn, size_t clientCertLen_bytesIn,
											const char* clientPrivateKeyIn, size_t clientPrivateKeyLen_bytesIn,
											int threadIdIn);

static void wifiManCb_onConnect(const char *const ssidIn, void* userVarIn);
static void wifiManCb_onDisconnect(void* userVarIn);

static void mqttClientCb_onConnect(cxa_mqtt_client_t *const clientIn, void* userVarIn);
static void mqttClientCb_onConnectFail(cxa_mqtt_client_t *const clientIn, cxa_mqtt_client_connectFailureReason_t reasonIn, void* userVarIn);
static void mqttClientCb_onDisconnect(cxa_mqtt_client_t *const clientIn, void* userVarIn);

static void stateCb_associating_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_connectStandOff_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_connectStandOff_state(cxa_stateMachine_t *const smIn, void *userVarIn);
static void stateCb_error_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);


// ********  local variable declarations *********
static cxa_mqtt_client_network_t mqttClient;
static cxa_led_t* led_conn;

static cxa_timeDiff_t td_connStandoff;
static uint32_t connStandoff_ms;

static cxa_stateMachine_t stateMachine;
static cxa_logger_t logger;

static char* mqtt_hostName;
static uint16_t mqtt_portNum;

static bool mqtt_useTls;
static const char* serverRootCert;
static size_t serverRootCertLen_bytes;
static const char* clientCert;
static size_t clientCertLen_bytes;
static const char* clientPrivateKey;
static size_t clientPrivateKeyLen_bytes;

static char* mqtt_username;
static uint8_t* mqtt_password;
static uint16_t mqtt_passwordLen_bytes;


// ******** global function implementations ********
void cxa_mqtt_connManager_init(cxa_led_t *const ledConnIn,
							   char *const hostNameIn, uint16_t portNumIn, bool useTlsIn,
							   char *const usernameIn, uint8_t *const passwordIn, uint16_t passwordLen_bytesIn,
							   int threadIdIn)
{
	cxa_mqtt_connManager_commonInit(ledConnIn,
									hostNameIn, portNumIn, useTlsIn,
									usernameIn, passwordIn, passwordLen_bytesIn,
									NULL, 0, NULL, 0, NULL, 0,
									threadIdIn);
}


void cxa_mqtt_connManager_init_clientCert(cxa_led_t *const ledConnIn,
										  char *const hostNameIn, uint16_t portNumIn,
										  const char* serverRootCertIn, size_t serverRootCertLen_bytesIn,
										  const char* clientCertIn, size_t clientCertLen_bytesIn,
										  const char* clientPrivateKeyIn, size_t clientPrivateKeyLen_bytesIn,
										  int threadIdIn)
{
	cxa_mqtt_connManager_commonInit(ledConnIn,
									hostNameIn, portNumIn, true,
									NULL, NULL, 0,
									serverRootCertIn, serverRootCertLen_bytesIn,
									clientCertIn, clientCertLen_bytesIn,
									clientPrivateKeyIn, clientPrivateKeyLen_bytesIn,
									threadIdIn);
}


cxa_mqtt_client_t* cxa_mqtt_connManager_getMqttClient(void)
{
	return &mqttClient.super;
}


// ******** local function implementations ********
static void cxa_mqtt_connManager_commonInit(cxa_led_t *const ledConnIn,
											char *const hostNameIn, uint16_t portNumIn, bool useTlsIn,
											char *const usernameIn, uint8_t *const passwordIn, uint16_t passwordLen_bytesIn,
											const char* serverRootCertIn, size_t serverRootCertLen_bytesIn,
											const char* clientCertIn, size_t clientCertLen_bytesIn,
											const char* clientPrivateKeyIn, size_t clientPrivateKeyLen_bytesIn,
											int threadIdIn)
{
	// save our references
	led_conn = ledConnIn;

	mqtt_hostName = hostNameIn;
	mqtt_portNum = portNumIn;

	mqtt_useTls = useTlsIn;
	serverRootCert = serverRootCertIn;
	serverRootCertLen_bytes = serverRootCertLen_bytesIn;
	clientCert = clientCertIn;
	clientCertLen_bytes = clientCertLen_bytesIn;
	clientPrivateKey = clientPrivateKeyIn;
	clientPrivateKeyLen_bytes = clientPrivateKeyLen_bytesIn;

	mqtt_username = usernameIn;
	mqtt_password = passwordIn;
	mqtt_passwordLen_bytes = passwordLen_bytesIn;

	// setup our connection standoff
	cxa_timeDiff_init(&td_connStandoff);
	srand(cxa_timeBase_getCount_us());

	// setup our logger
	cxa_logger_init(&logger, "connectionManager");

	// setup our state machine
	cxa_stateMachine_init(&stateMachine, "mqttConnMan", threadIdIn);
	cxa_stateMachine_addState(&stateMachine, STATE_ASSOCIATING, "assoc", stateCb_associating_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_CONNECTING, "connecting", stateCb_connecting_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_CONNECTED, "connected", stateCb_connected_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_CONNECT_STANDOFF, "standOff", stateCb_connectStandOff_enter, stateCb_connectStandOff_state, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_ERROR, "error" , stateCb_error_enter, NULL, NULL, NULL);
	cxa_stateMachine_setInitialState(&stateMachine, STATE_ASSOCIATING);

	// setup our WiFi
	cxa_network_wifiManager_addListener(NULL, NULL, NULL, wifiManCb_onConnect, wifiManCb_onDisconnect, NULL, NULL, NULL);

	// and our mqtt client
	cxa_mqtt_client_network_init(&mqttClient, cxa_uniqueId_getHexString(), threadIdIn);
	cxa_mqtt_client_addListener(&mqttClient.super, mqttClientCb_onConnect, mqttClientCb_onConnectFail, mqttClientCb_onDisconnect, NULL, NULL);
}


static void wifiManCb_onConnect(const char *const ssidIn, void* userVarIn)
{
	cxa_logger_info(&logger, "wifi connected");
	cxa_stateMachine_transition(&stateMachine, STATE_CONNECTING);
}


static void wifiManCb_onDisconnect(void* userVarIn)
{
	cxa_logger_warn(&logger, "wifi disconnected");

	// ensure we are disconnected regardless
	cxa_mqtt_client_disconnect(&mqttClient.super);

	cxa_stateMachine_transition(&stateMachine, STATE_ASSOCIATING);
}


static void mqttClientCb_onConnect(cxa_mqtt_client_t *const clientIn, void* userVarIn)
{
	cxa_stateMachine_transition(&stateMachine, STATE_CONNECTED);
}


static void mqttClientCb_onConnectFail(cxa_mqtt_client_t *const clientIn, cxa_mqtt_client_connectFailureReason_t reasonIn, void* userVarIn)
{
	if( cxa_stateMachine_getCurrentState(&stateMachine) == STATE_CONNECTING )
	{
		cxa_logger_warn(&logger, "connection failed: %d", reasonIn);
		cxa_stateMachine_transition(&stateMachine, STATE_CONNECT_STANDOFF);
	}
}


static void mqttClientCb_onDisconnect(cxa_mqtt_client_t *const clientIn, void* userVarIn)
{
	if( cxa_stateMachine_getCurrentState(&stateMachine) == STATE_CONNECTED )
	{
		cxa_logger_warn(&logger, "disconnected");
		cxa_stateMachine_transition(&stateMachine, STATE_CONNECT_STANDOFF);
	}
}


static void stateCb_associating_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "associating");
	if( led_conn != NULL ) cxa_led_blink(led_conn, BLINKPERIODMS_ON_ASSOC,  BLINKPERIODMS_OFF_ASSOC);
}


static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "connecting");

	if( led_conn != NULL ) cxa_led_blink(led_conn, BLINKPERIODMS_ON_CONNECTING, BLINKPERIODMS_OFF_CONNECTING);

	if( clientCert != NULL )
	{
		if( !cxa_mqtt_client_network_connectToHost_clientCert(&mqttClient, mqtt_hostName, mqtt_portNum,
															  serverRootCert, serverRootCertLen_bytes,
															  clientCert, clientCertLen_bytes,
															  clientPrivateKey, clientPrivateKeyLen_bytes) )
		{
			cxa_logger_warn(&logger, "failed to start network connection");
			return;
		}
	}
	else
	{
		if( !cxa_mqtt_client_network_connectToHost(&mqttClient, mqtt_hostName, mqtt_portNum, mqtt_useTls, mqtt_username, mqtt_password, mqtt_passwordLen_bytes) )
		{
			cxa_logger_warn(&logger, "failed to start network connection");
			return;
		}
	}
}


static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "connected");

	if( led_conn != NULL ) cxa_led_blink(led_conn, BLINKPERIODMS_ON_CONNECTED, BLINKPERIODMS_OFF_CONNECTED);
}


static void stateCb_connectStandOff_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	connStandoff_ms = rand() % 1000 + 500;
	cxa_logger_info(&logger, "retry connection after %d ms", connStandoff_ms);
	cxa_timeDiff_setStartTime_now(&td_connStandoff);
}


static void stateCb_connectStandOff_state(cxa_stateMachine_t *const smIn, void *userVarIn)
{
	if( cxa_timeDiff_isElapsed_ms(&td_connStandoff, connStandoff_ms) )
	{
		cxa_stateMachine_transition(&stateMachine, STATE_CONNECTING);
	}
}


static void stateCb_error_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "unrecoverable error");

	if( led_conn != NULL ) cxa_led_blink(led_conn, BLINKPERIODMS_ON_ERROR, BLINKPERIODMS_OFF_ERROR);
}
