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
								cxa_network_tcpServer_scm_listen_t scm_listenIn,
								cxa_network_tcpServer_scm_stopListening_t scm_stopListeningIn)
{
	cxa_assert(tcpServerIn);
	cxa_assert(scm_listenIn);

	// save our references
	tcpServerIn->scm_listen = scm_listenIn;
	tcpServerIn->scm_stopListening = scm_stopListeningIn;

	// setup our logger
	cxa_logger_init(&tcpServerIn->logger, "netServer");

	// setup our listener array
	cxa_array_initStd(&tcpServerIn->listeners, tcpServerIn->listeners_raw);
}


void cxa_network_tcpServer_addListener(cxa_network_tcpServer_t *const tcpServerIn,
									   cxa_network_tcpServer_cb_onConnect_t cb_onConnectIn,
									   void* userVarIn)
{
	cxa_assert(tcpServerIn);

	cxa_network_tcpServer_listenerEntry_t newEntry = {
			.cb_onConnect=cb_onConnectIn,
			.userVar=userVarIn
	};
	cxa_assert( cxa_array_append(&tcpServerIn->listeners, &newEntry) );
}


bool cxa_network_tcpServer_listen(cxa_network_tcpServer_t *const tcpServerIn, uint16_t portNumIn)
{
	cxa_assert(tcpServerIn);

	return tcpServerIn->scm_listen(tcpServerIn, portNumIn);
}


void cxa_network_tcpServer_stopListening(cxa_network_tcpServer_t *const tcpServerIn)
{
	cxa_assert(tcpServerIn);

	return tcpServerIn->scm_stopListening(tcpServerIn);
}


void cxa_network_tcpServer_notifyConnect(cxa_network_tcpServer_t *const tcpServerIn, cxa_network_tcpServer_connectedClient_t* clientIn)
{
	cxa_assert(tcpServerIn);

	cxa_array_iterate(&tcpServerIn->listeners, currListener, cxa_network_tcpServer_listenerEntry_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onConnect != NULL ) currListener->cb_onConnect(tcpServerIn, clientIn, currListener->userVar);
	}
}


// ******** local function implementations ********
