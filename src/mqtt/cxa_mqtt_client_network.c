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
#include "cxa_mqtt_client_network.h"


// ******** includes ********
#include <cxa_assert.h>
#include <cxa_network_factory.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define NET_CONNECT_TIMEOUT_MS				10000
#define KEEPALIVE_TIMEOUT_S					10


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_onDisconnect(cxa_mqtt_client_t* const superIn);
static void cb_network_onConnect(cxa_network_tcpClient_t *const superIn, void* userVarIn);
static void cb_network_onConnectFail(cxa_network_tcpClient_t *const tcpClientIn, void* userVarIn);
static void cb_network_onDisconnect(cxa_network_tcpClient_t *const superIn, void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_mqtt_client_network_init(cxa_mqtt_client_network_t *const clientIn, char *const clientIdIn)
{
	cxa_assert(clientIn);

	// initialize our network client
	clientIn->netClient = cxa_network_factory_reserveTcpClient();
	cxa_assert(clientIn->netClient);
	cxa_network_tcpClient_addListener(clientIn->netClient,
									  cb_network_onConnect, cb_network_onConnectFail, cb_network_onDisconnect,
									  (void*)clientIn);

	// initialize our super class
	cxa_mqtt_client_init(&clientIn->super, cxa_network_tcpClient_getIoStream(clientIn->netClient), KEEPALIVE_TIMEOUT_S, clientIdIn);

	// setup our overrides
	clientIn->super.scm_onDisconnect = scm_onDisconnect;
}


bool cxa_mqtt_client_network_connectToHost(cxa_mqtt_client_network_t *const clientIn, char *const hostNameIn, uint16_t portNumIn,
										   char *const usernameIn, uint8_t *const passwordIn, uint16_t passwordLen_bytesIn)
{
	cxa_assert(clientIn);
	cxa_assert(clientIn->netClient);
	cxa_assert(hostNameIn);

	// have to save these for later
	clientIn->username = usernameIn;
	clientIn->password = passwordIn;
	clientIn->passwordLen_bytes = passwordLen_bytesIn;

	return cxa_network_tcpClient_connectToHost(clientIn->netClient, hostNameIn, portNumIn, NET_CONNECT_TIMEOUT_MS);
}


// ******** local function implementations ********
static void scm_onDisconnect(cxa_mqtt_client_t* const superIn)
{
	cxa_mqtt_client_network_t *clientIn = (cxa_mqtt_client_network_t*)superIn;
	cxa_assert(clientIn);

	cxa_network_tcpClient_disconnect(clientIn->netClient);
}


static void cb_network_onConnect(cxa_network_tcpClient_t *const superIn, void* userVarIn)
{
	cxa_mqtt_client_network_t *clientIn = (cxa_mqtt_client_network_t*)userVarIn;
	cxa_assert(clientIn);

	// we're connected (network-wise), start the mqtt connect
	cxa_mqtt_client_connect(&clientIn->super, clientIn->username, clientIn->password, clientIn->passwordLen_bytes);
}


static void cb_network_onConnectFail(cxa_network_tcpClient_t *const tcpClientIn, void* userVarIn)
{
	cxa_mqtt_client_network_t *clientIn = (cxa_mqtt_client_network_t*)userVarIn;
	cxa_assert(clientIn);

	cxa_mqtt_client_notify_connectFail(&clientIn->super, CXA_MQTT_CLIENT_CONNECTFAIL_REASON_NETWORK);
}


static void cb_network_onDisconnect(cxa_network_tcpClient_t *const superIn, void* userVarIn)
{
	cxa_mqtt_client_network_t *clientIn = (cxa_mqtt_client_network_t*)userVarIn;
	cxa_assert(clientIn);

	cxa_mqtt_client_notify_disconnect(&clientIn->super);
}

