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
#include <cxa_network_tcpServer.h>

// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_network_tcpServer_init(cxa_network_tcpServer_t *const tcpServerIn,
								cxa_network_tcpServer_cb_listen_t cb_listenIn,
								cxa_network_tcpServer_cb_disconnectFromClient_t cb_disconnectFromClientIn,
								cxa_network_tcpServer_cb_isConnected_t cb_isConnected)
{
	cxa_assert(tcpServerIn);
	cxa_assert(cb_listenIn);
	cxa_assert(cb_disconnectFromClientIn);
	cxa_assert(cb_isConnected);

	// save our references
	tcpServerIn->cb_listen = cb_listenIn;
	tcpServerIn->cb_disconnectFromClient = cb_disconnectFromClientIn;
	tcpServerIn->cb_isConnected = cb_isConnected;

	// setup our logger
	cxa_logger_init(&tcpServerIn->logger, "netClient");

	// setup our listener array
	cxa_array_initStd(&tcpServerIn->listeners, tcpServerIn->listeners_raw);

	// setup our ioStream (but the actual server is responsible for binding it)
	cxa_ioStream_init(&tcpServerIn->ioStream);
}


void cxa_network_tcpServer_addListener(cxa_network_tcpServer_t *const tcpServerIn,
									   cxa_network_tcpServer_cb_onConnect_t cb_onConnectIn,
									   cxa_network_tcpServer_cb_onDisconnect_t cb_onDisconnectIn,
									   void* userVarIn)
{
	cxa_assert(tcpServerIn);

	cxa_network_tcpServer_listenerEntry_t newEntry = {
			.cb_onConnect=cb_onConnectIn,
			.cb_onDisconnect=cb_onDisconnectIn,
			.userVar=userVarIn
	};
	cxa_assert( cxa_array_append(&tcpServerIn->listeners, &newEntry) );
}


bool cxa_network_tcpServer_listen(cxa_network_tcpServer_t *const tcpServerIn, uint16_t portNumIn)
{
	cxa_assert(tcpServerIn);

	return tcpServerIn->cb_listen(tcpServerIn, portNumIn);
}


void cxa_network_tcpServer_disconnect(cxa_network_tcpServer_t *const tcpServerIn)
{
	cxa_assert(tcpServerIn);

	return tcpServerIn->cb_disconnectFromClient(tcpServerIn);
}


bool cxa_network_tcpServer_isConnected(cxa_network_tcpServer_t *const tcpServerIn)
{
	cxa_assert(tcpServerIn);

	return tcpServerIn->cb_isConnected(tcpServerIn);
}


cxa_ioStream_t* cxa_network_tcpServer_getIoStream(cxa_network_tcpServer_t *const tcpServerIn)
{
	cxa_assert(tcpServerIn);

	return &tcpServerIn->ioStream;
}


// ******** local function implementations ********
