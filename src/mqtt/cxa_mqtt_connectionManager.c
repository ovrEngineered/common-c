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
#include <cxa_stateMachine.h>
#include <cxa_timeBase.h>
#include <cxa_uniqueId.h>

#include <cxa_config.h>

#define CXA_LOG_LEVEL					CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********
typedef enum
{
	STATE_WAIT_CREDS,
	STATE_ASSOCIATING,
	STATE_CONNECTING,
	STATE_CONNECTED,
	STATE_CONNECT_STANDOFF,
}state_t;


// ******** local function prototypes ********
static void wifiManCb_onAssociated(const char *const ssidIn, void* userVarIn);
static void wifiManCb_onUnassociated(void* userVarIn);

static void mqttClientCb_onConnect(cxa_mqtt_client_t *const clientIn, void* userVarIn);
static void mqttClientCb_onConnectFail(cxa_mqtt_client_t *const clientIn, cxa_mqtt_client_connectFailureReason_t reasonIn, void* userVarIn);
static void mqttClientCb_onDisconnect(cxa_mqtt_client_t *const clientIn, void* userVarIn);

static void stateCb_waitCredentials_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_associating_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_connectStandOff_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn);
static void stateCb_connectStandOff_state(cxa_stateMachine_t *const smIn, void *userVarIn);


// ********  local variable declarations *********
static cxa_mqtt_client_network_t mqttClient;

static cxa_timeDiff_t td_connStandoff;
static uint32_t connStandoff_ms;

static cxa_stateMachine_t stateMachine;
static cxa_logger_t logger;

static char* mqtt_hostName;
static uint16_t mqtt_portNum;

static bool areCredsSet = false;
static const char* serverRootCert;
static size_t serverRootCertLen_bytes;
static const char* clientCert;
static size_t clientCertLen_bytes;
static const char* clientPrivateKey;
static size_t clientPrivateKeyLen_bytes;


// ******** global function implementations ********
void cxa_mqtt_connManager_init(char *const hostNameIn, uint16_t portNumIn, int threadIdIn)
{
	// save our references
	mqtt_hostName = hostNameIn;
	mqtt_portNum = portNumIn;

	// setup our connection standoff
	cxa_timeDiff_init(&td_connStandoff);
	srand(cxa_timeBase_getCount_us());

	// setup our logger
	cxa_logger_init(&logger, "connectionManager");

	// setup our state machine
	cxa_stateMachine_init(&stateMachine, "mqttConnMan", threadIdIn);
	cxa_stateMachine_addState(&stateMachine, STATE_ASSOCIATING, "assoc", stateCb_associating_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_WAIT_CREDS, "waitForCreds", stateCb_waitCredentials_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_CONNECTING, "connecting", stateCb_connecting_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_CONNECTED, "connected", stateCb_connected_enter, NULL, NULL, NULL);
	cxa_stateMachine_addState(&stateMachine, STATE_CONNECT_STANDOFF, "standOff", stateCb_connectStandOff_enter, stateCb_connectStandOff_state, NULL, NULL);
	cxa_stateMachine_setInitialState(&stateMachine, STATE_ASSOCIATING);

	// setup our WiFi
	cxa_network_wifiManager_addListener(NULL, NULL, NULL, wifiManCb_onAssociated, wifiManCb_onUnassociated, NULL, NULL);

	// and our mqtt client
	cxa_mqtt_client_network_init(&mqttClient, cxa_uniqueId_getHexString(), threadIdIn);
	cxa_mqtt_client_addListener(&mqttClient.super, mqttClientCb_onConnect, mqttClientCb_onConnectFail, mqttClientCb_onDisconnect, NULL, NULL);
}


void cxa_mqtt_connManager_setTlsCredentials(const char* serverRootCertIn, size_t serverRootCertLen_bytesIn,
		  	  	  	  	  	  	  	  	    const char* clientCertIn, size_t clientCertLen_bytesIn,
											const char* clientPrivateKeyIn, size_t clientPrivateKeyLen_bytesIn)
{
	cxa_assert(serverRootCertIn);
	cxa_assert(clientCertIn);
	cxa_assert(clientPrivateKeyIn);

	// store our references
	serverRootCert = serverRootCertIn;
	serverRootCertLen_bytes = serverRootCertLen_bytesIn;
	clientCert = clientCertIn;
	clientCertLen_bytes = clientCertLen_bytesIn;
	clientPrivateKey = clientPrivateKeyIn;
	clientPrivateKeyLen_bytes = clientCertLen_bytesIn;

	areCredsSet = true;

	// if we're waiting for credentials, we're already associated...start connecting
	if( cxa_stateMachine_getCurrentState(&stateMachine) == STATE_WAIT_CREDS )
	{
		cxa_stateMachine_transition(&stateMachine, STATE_CONNECTING);
		return;
	}
}


bool cxa_mqtt_connManager_areCredentialsSet(void)
{
	return areCredsSet;
}


cxa_mqtt_client_t* cxa_mqtt_connManager_getMqttClient(void)
{
	return &mqttClient.super;
}


// ******** local function implementations ********
static void wifiManCb_onAssociated(const char *const ssidIn, void* userVarIn)
{
	cxa_logger_info(&logger, "wifi associated");
	cxa_stateMachine_transition(&stateMachine, (areCredsSet) ? STATE_CONNECTING : STATE_WAIT_CREDS);
}


static void wifiManCb_onUnassociated(void* userVarIn)
{
	cxa_logger_warn(&logger, "wifi unassociated");

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


static void stateCb_waitCredentials_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "waiting for credentials");
}


static void stateCb_associating_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "associating");
}


static void stateCb_connecting_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "connecting");

	// make _sure_we have credentials
	if( !areCredsSet )
	{
		cxa_stateMachine_transition(&stateMachine, STATE_WAIT_CREDS);
		return;
	}

	// if we made it here, we have the proper credentials...try to connect
	if( !cxa_mqtt_client_network_connectToHost_clientCert(&mqttClient, mqtt_hostName, mqtt_portNum,
														  serverRootCert, serverRootCertLen_bytes,
														  clientCert, clientCertLen_bytes,
														  clientPrivateKey, clientPrivateKeyLen_bytes) )
	{
		cxa_logger_warn(&logger, "failed to start network connection");
		cxa_stateMachine_transition(&stateMachine, STATE_CONNECT_STANDOFF);
		return;
	}
}


static void stateCb_connected_enter(cxa_stateMachine_t *const smIn, int nextStateIdIn, void *userVarIn)
{
	cxa_logger_info(&logger, "connected");
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
