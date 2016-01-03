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
#include <cxa_assert.h>
#include <cxa_network_tcpClient.h>

#define CXA_LOG_LEVEL CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_network_tcpClient_init(cxa_network_tcpClient_t *const netClientIn,
							 cxa_network_tcpClient_cb_connectToHost_t cb_connToHostIn,
							 cxa_network_tcpClient_cb_disconnectFromHost_t cb_disconnectIn,
							 cxa_network_tcpClient_cb_isConnected_t cb_isConnected)
{
	cxa_assert(netClientIn);
	cxa_assert(cb_connToHostIn);
	cxa_assert(cb_disconnectIn);
	cxa_assert(cb_isConnected);

	// save our references
	netClientIn->cb_connToHost = cb_connToHostIn;
	netClientIn->cb_disconnect = cb_disconnectIn;
	netClientIn->cb_isConnected = cb_isConnected;

	// setup our logger
	cxa_logger_init(&netClientIn->logger, "netClient");

	// setup our timediff for future use
	cxa_timeDiff_init(&netClientIn->td_genPurp, true);

	// setup our listener array
	cxa_array_initStd(&netClientIn->listeners, netClientIn->listeners_raw);

	// setup our ioStream (but the actual client is responsible for binding it)
	cxa_ioStream_init(&netClientIn->ioStream);
}


void cxa_network_tcpClient_addListener(cxa_network_tcpClient_t *const netClientIn,
									cxa_network_tcpClient_cb_onConnect_t cb_onConnectIn,
									cxa_network_tcpClient_cb_onConnectFail_t cb_onConnectFailIn,
									cxa_network_tcpClient_cb_onDisconnect_t cb_onDisconnectIn,
									void* userVarIn)
{
	cxa_assert(netClientIn);

	cxa_network_tcpClient_listenerEntry_t newEntry = {.cb_onConnect=cb_onConnectIn, .cb_onConnectFail=cb_onConnectFailIn, .cb_onDisconnect=cb_onDisconnectIn, .userVar=userVarIn};
	cxa_assert( cxa_array_append(&netClientIn->listeners, &newEntry) );
}


bool cxa_network_tcpClient_connectToHost(cxa_network_tcpClient_t *const netClientIn, char *const hostNameIn, uint16_t portNumIn, uint32_t timeout_msIn, bool autoReconnectIn)
{
	cxa_assert(netClientIn);
	cxa_assert(hostNameIn);

	return netClientIn->cb_connToHost(netClientIn, hostNameIn, portNumIn, timeout_msIn, autoReconnectIn);
}


void cxa_network_tcpClient_disconnect(cxa_network_tcpClient_t *const netClientIn)
{
	cxa_assert(netClientIn);

	netClientIn->cb_disconnect(netClientIn);
}


bool cxa_network_tcpClient_isConnected(cxa_network_tcpClient_t *const netClientIn)
{
	cxa_assert(netClientIn);

	return netClientIn->cb_isConnected(netClientIn);
}


cxa_ioStream_t* cxa_network_tcpClient_getIoStream(cxa_network_tcpClient_t *const netClientIn)
{
	cxa_assert(netClientIn);

	return &netClientIn->ioStream;
}


// ******** local function implementations ********
