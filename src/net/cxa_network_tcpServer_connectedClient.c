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
#include <cxa_network_tcpServer_connectedClient.h>


// ******** includes ********
#include <cxa_assert.h>

#define CXA_LOG_LEVEL CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_network_tcpServer_connectedClient_initUnbound(cxa_network_tcpServer_connectedClient_t *const ccIn,
							cxa_network_tcpServer_connectedClient_scm_isBound_t scm_isBoundIn,
							cxa_network_tcpServer_connectedClient_scm_unbindAndClose_t scm_unbindAndCloseIn,
							cxa_network_tcpServer_connectedClient_scm_getDescriptiveString_t scm_getDescriptiveStringIn)
{
	cxa_assert(ccIn);
	cxa_assert(scm_isBoundIn);
	cxa_assert(scm_unbindAndCloseIn);
	cxa_assert(scm_getDescriptiveStringIn);

	ccIn->scm_isBound = scm_isBoundIn;
	ccIn->scm_unbindAndClose = scm_unbindAndCloseIn;
	ccIn->scm_getDescriptiveString = scm_getDescriptiveStringIn;

	cxa_logger_init(&ccIn->logger, "tcpConnClient");

	cxa_array_initStd(&ccIn->listeners, ccIn->listeners_raw);

	cxa_ioStream_init(&ccIn->ioStream);
}


void cxa_network_tcpServer_connectedClient_addListener(cxa_network_tcpServer_connectedClient_t *const ccIn,
							cxa_network_tcpServer_connectedClient_cb_onDisconnect_t cb_onDisconnectIn,
							void* userVarIn)
{
	cxa_assert(ccIn);

	cxa_network_tcpServer_connectedClient_listenerEntry_t newEntry = {
			.cb_onDisconnect = cb_onDisconnectIn,
			.userVar = userVarIn
	};

	cxa_assert(cxa_array_append(&ccIn->listeners, &newEntry));
}


bool cxa_network_tcpServer_connectedClient_isBound(cxa_network_tcpServer_connectedClient_t *const ccIn)
{
	cxa_assert(ccIn);
	cxa_assert(ccIn->scm_isBound);

	return ccIn->scm_isBound(ccIn);
}


void cxa_network_tcpServer_connectedClient_unbindAndClose(cxa_network_tcpServer_connectedClient_t *const ccIn)
{
	cxa_assert(ccIn);
	cxa_assert(ccIn->scm_unbindAndClose);

	ccIn->scm_unbindAndClose(ccIn);
}


cxa_ioStream_t* cxa_network_tcpServer_connectedClient_getIoStream(cxa_network_tcpServer_connectedClient_t *const ccIn)
{
	cxa_assert(ccIn);

	return &ccIn->ioStream;
}


char* cxa_network_tcpServer_connectedClient_getDescriptiveString(cxa_network_tcpServer_connectedClient_t *const ccIn)
{
	cxa_assert(ccIn);
	cxa_assert(ccIn->scm_getDescriptiveString);

	return ccIn->scm_getDescriptiveString(ccIn);
}


void cxa_network_tcpServer_connectedClient_notifyDisconnected(cxa_network_tcpServer_connectedClient_t *const ccIn)
{
	cxa_assert(ccIn);

	cxa_array_iterate(&ccIn->listeners, currListener, cxa_network_tcpServer_connectedClient_listenerEntry_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onDisconnect != NULL ) currListener->cb_onDisconnect(ccIn, currListener->userVar);
	}
}


// ******** local function implementations ********
