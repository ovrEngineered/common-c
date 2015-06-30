/**
 * Copyright 2013 opencxa.org
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
#include "cxa_rpc_nodeRemote.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <string.h>
#include <cxa_assert.h>
#include <cxa_rpc_node.h>
#include <cxa_rpc_messageFactory.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define PROTO_VERSION					2
#define ASK_FOR_NAME_TIMEOUT_MS			5000
#define LINK_MANAGEMENT_DEST			"_linkMan"
#define LMM_GET_NAME					"_getName"
#define LM_GET_NAME_ID					1234


// ******** local type definitions ********


// ******** local function prototypes ********
static bool isUpstream(cxa_rpc_nodeRemote_t *const nrIn);

static void handleMessage_upstream(cxa_rpc_messageHandler_t *const handlerIn, cxa_rpc_message_t *const msgIn);
static void handleMessage_downstream(cxa_rpc_messageHandler_t *const handlerIn, cxa_rpc_message_t *const msgIn);

static void messageReceived_cb(cxa_rpc_message_t *const msgIn, void *const userVarIn);

static void handleLinkManagement_upstream(cxa_rpc_nodeRemote_t *const nrIn, cxa_rpc_message_t *const msgIn);
static void handleLinkManagement_downstream(cxa_rpc_nodeRemote_t *const nrIn, cxa_rpc_message_t *const msgIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_rpc_nodeRemote_init_upstream(cxa_rpc_nodeRemote_t *const nrIn, cxa_ioStream_t *const ioStreamIn, cxa_timeBase_t *const timeBaseIn)
{
	cxa_assert(nrIn);
	cxa_assert(ioStreamIn);
	cxa_assert(timeBaseIn);

	// initialize our super class (but we don't have a name yet)
	cxa_rpc_messageHandler_init(&nrIn->super, handleMessage_upstream, handleMessage_downstream);

	// setup our initial state
	nrIn->downstreamSubNode = NULL;
	cxa_timeDiff_init(&nrIn->td_askForName, timeBaseIn, false);
	cxa_array_initStd(&nrIn->linkListeners, nrIn->linkListeners_raw);

	// setup our logger
	cxa_logger_vinit(&nrIn->logger, "rpcNr_%p", nrIn);

	// initialize our protocol parser
	cxa_rpc_protocolParser_init(&nrIn->protocolParser, PROTO_VERSION, ioStreamIn);
	cxa_rpc_protocolParser_addMessageListener(&nrIn->protocolParser, messageReceived_cb, nrIn);
}


bool cxa_rpc_nodeRemote_init_downstream(cxa_rpc_nodeRemote_t *const nrIn, cxa_ioStream_t *const ioStreamIn, cxa_rpc_node_t *const subNodeIn)
{
	cxa_assert(nrIn);
	cxa_assert(ioStreamIn);

	// initialize our super class (but we we won't use our name)
	cxa_rpc_messageHandler_init(&nrIn->super, handleMessage_upstream, handleMessage_downstream);

	// setup our initial state
	nrIn->downstreamSubNode = subNodeIn;
	cxa_array_initStd(&nrIn->linkListeners, nrIn->linkListeners_raw);

	// setup our logger
	cxa_logger_vinit(&nrIn->logger, "rpcNr_%p", nrIn);

	// initialize our protocol parser
	cxa_rpc_protocolParser_init(&nrIn->protocolParser, PROTO_VERSION, ioStreamIn);
	cxa_rpc_protocolParser_addMessageListener(&nrIn->protocolParser, messageReceived_cb, nrIn);

	// set ourselves as the downstream node's parent
	if( subNodeIn->super.parent != NULL )
	{
		cxa_logger_warn(&nrIn->logger, "attempted downstream node %p already has parent", subNodeIn);
		return false;
	}

	// make sure we're not trying to add the global root as a subnode
	if( subNodeIn->isGlobalRoot )
	{
		cxa_logger_warn(&nrIn->logger, "cannot add the global root as downstream subnode");
		return false;
	}

	// if we made it here, we're good to add
	subNodeIn->super.parent = &nrIn->super;

	cxa_logger_debug(&nrIn->logger, "owns node '%s' @ [%p]", cxa_rpc_messageHandler_getName(&subNodeIn->super), subNodeIn);
	return true;
}


bool cxa_rpc_nodeRemote_addLinkListener(cxa_rpc_nodeRemote_t *const nrIn, cxa_rpc_nodeRemote_cb_linkEstablished_t cb_linkEstablishedIn, void *const userVarIn)
{
	cxa_assert(nrIn);
	cxa_assert(cb_linkEstablishedIn);

	cxa_rpc_nodeRemote_linkListener_t newListener = {.cb_linkEstablished=cb_linkEstablishedIn, .userVar=userVarIn};
	return cxa_array_append(&nrIn->linkListeners, &newListener);
}


void cxa_rpc_nodeRemote_update(cxa_rpc_nodeRemote_t *const nrIn)
{
	cxa_assert(nrIn);

	// see if we need to ask for a name...
	if( isUpstream(nrIn) && !cxa_rpc_messageHandler_getName(&nrIn->super) && cxa_timeDiff_isElaped_recurring_ms(&nrIn->td_askForName, ASK_FOR_NAME_TIMEOUT_MS) )
	{
		cxa_rpc_message_t* nameReqMsg = cxa_rpc_messageFactory_getFreeMessage_empty();
		if( !nameReqMsg || !cxa_rpc_message_initRequest(nameReqMsg, LINK_MANAGEMENT_DEST, LMM_GET_NAME, NULL, 0) ||
				!cxa_rpc_message_setId(nameReqMsg, LM_GET_NAME_ID) || !cxa_rpc_message_prependNodeNameToSource(nameReqMsg, LMM_GET_NAME) )
		{
			cxa_logger_warn(&nrIn->logger, "error getting/initializing name request, will retry");
			cxa_rpc_messageFactory_decrementMessageRefCount(nameReqMsg);
			return;
		}

		if( !cxa_rpc_protocolParser_writeMessage(&nrIn->protocolParser, nameReqMsg) )
		{
			cxa_logger_warn(&nrIn->logger, "error writing name request, will retry");
			cxa_rpc_messageFactory_decrementMessageRefCount(nameReqMsg);
			return;
		}

		cxa_logger_debug(&nrIn->logger, "sent name request");
		cxa_rpc_messageFactory_decrementMessageRefCount(nameReqMsg);
	}

	cxa_rpc_protocolParser_update(&nrIn->protocolParser);
}


// ******** local function implementations ********
static bool isUpstream(cxa_rpc_nodeRemote_t *const nrIn)
{
	cxa_assert(nrIn);

	return (nrIn->downstreamSubNode == NULL);
}


static void handleMessage_upstream(cxa_rpc_messageHandler_t *const handlerIn, cxa_rpc_message_t *const msgIn)
{
	cxa_assert(handlerIn);
	cxa_rpc_nodeRemote_t* nrIn = (cxa_rpc_nodeRemote_t*)handlerIn;
	if( !msgIn ) return;

	// this function should only be called on the downstream side of the connection
	if( isUpstream(nrIn) )
	{
		cxa_logger_warn(&nrIn->logger, "handleUpstream(%p): called on upstream side, dropping message", msgIn);
		return;
	}

	// we have a message that we need to send via our ioStream
	if( !cxa_rpc_protocolParser_writeMessage(&nrIn->protocolParser, msgIn))
	{
		cxa_logger_warn(&nrIn->logger, "handleUpstream(%p): protocolParser reports write error, dropping message", msgIn);
		return;
	}
}


static void handleMessage_downstream(cxa_rpc_messageHandler_t *const handlerIn, cxa_rpc_message_t *const msgIn)
{
	cxa_assert(handlerIn);
	cxa_rpc_nodeRemote_t* nrIn = (cxa_rpc_nodeRemote_t*)handlerIn;
	if( !msgIn ) return;

	// this function should only be called on the upstream side of the connection
	if( !isUpstream(nrIn) )
	{
		cxa_logger_warn(&nrIn->logger, "handleDownstream(%p): called on downstream side, dropping message", msgIn);
		return;
	}

	// we have a message that we need to send via our ioStream
	if( !cxa_rpc_protocolParser_writeMessage(&nrIn->protocolParser, msgIn))
	{
		cxa_logger_warn(&nrIn->logger, "handleDownstream(%p): protocolParser reports write error, dropping message", msgIn);
		return;
	}
}


static void messageReceived_cb(cxa_rpc_message_t *const msgIn, void *const userVarIn)
{
	cxa_assert(msgIn);
	cxa_assert(userVarIn);

	cxa_rpc_nodeRemote_t* nrIn = (cxa_rpc_nodeRemote_t*)userVarIn;

	// check for our exclusive link management messages...
	char* dest = cxa_rpc_message_getDestination(msgIn);
	if( (dest != NULL) && (strcmp(dest, LINK_MANAGEMENT_DEST) == 0))
	{
		// this is a link management message...process locally and discard
		if( isUpstream(nrIn) )
		{
			handleLinkManagement_upstream(nrIn, msgIn);
		}
		else
		{
			handleLinkManagement_downstream(nrIn, msgIn);
		}
		return;
	}

	// normal operation...depends what side we are on...
	if( isUpstream(nrIn) )
	{
		// pass to our parent for processing
		cxa_rpc_messageHandler_handleUpstream(nrIn->super.parent, msgIn);
	}
	else
	{
		// pass to our subNode for processing
		cxa_rpc_messageFactory_decrementMessageRefCount(msgIn);
		cxa_rpc_messageHandler_handleDownstream(&nrIn->downstreamSubNode->super, msgIn);
	}
}


static void handleLinkManagement_upstream(cxa_rpc_nodeRemote_t *const nrIn, cxa_rpc_message_t *const msgIn)
{
	cxa_assert(nrIn);
	cxa_assert(msgIn);

	if( (cxa_rpc_message_getType(msgIn) == CXA_RPC_MESSAGE_TYPE_RESPONSE) && (cxa_rpc_message_getId(msgIn) == LM_GET_NAME_ID) )
	{

		// this a response for a name...
		cxa_linkedField_t* retParams = cxa_rpc_message_getParams(msgIn);
		if( retParams == NULL )
		{
			cxa_logger_warn(&nrIn->logger, "no return params for name response");
			return;
		}

		char* newName = (char*)cxa_linkedField_get_pointerToIndex(retParams, 0);
		if( (newName == NULL) || (strlen(newName) == 0) )
		{
			cxa_logger_warn(&nrIn->logger, "name response strlen is 0");
			return;
		}

		// got a good name
		cxa_logger_debug(&nrIn->logger, "name is now '%s'", newName);
		cxa_logger_vinit(&nrIn->logger, "rpcNodeRemote_%s", newName);
		cxa_rpc_messageHandler_setName(&nrIn->super, newName);

		// notify our listeners
		cxa_array_iterate(&nrIn->linkListeners, currListener, cxa_rpc_nodeRemote_linkListener_t)
		{
			if( currListener == NULL ) continue;
			if( currListener->cb_linkEstablished != NULL ) currListener->cb_linkEstablished(nrIn, currListener->userVar);
		}
	}
	else cxa_logger_warn(&nrIn->logger, "unknown upstream linkManagement message received");
}


static void handleLinkManagement_downstream(cxa_rpc_nodeRemote_t *const nrIn, cxa_rpc_message_t *const msgIn)
{
	cxa_assert(nrIn);
	cxa_assert(msgIn);

	char *method = NULL;
	if( (cxa_rpc_message_getType(msgIn) == CXA_RPC_MESSAGE_TYPE_REQUEST) &&
			((method = cxa_rpc_message_getMethod(msgIn)) != NULL) && (strcmp(method, LMM_GET_NAME) == 0) )
	{
		// this is a request for a name...return the name of our subNode
		cxa_rpc_message_t* respMsg = cxa_rpc_messageFactory_getFreeMessage_empty();
		cxa_linkedField_t* retParams = NULL;
		const char* subNodeName = cxa_rpc_messageHandler_getName(&nrIn->downstreamSubNode->super);
		if( (respMsg == NULL) || !cxa_rpc_message_initResponse(respMsg, LINK_MANAGEMENT_DEST, cxa_rpc_message_getId(msgIn)) ||
				!cxa_rpc_message_prependNodeNameToSource(respMsg, LMM_GET_NAME) ||
				((retParams = cxa_rpc_message_getParams(respMsg)) == NULL) || (subNodeName == NULL) ||
				!cxa_linkedField_append_cString(retParams, subNodeName) )
		{
			cxa_logger_warn(&nrIn->logger, "error getting/initializing name response");
			cxa_rpc_messageFactory_decrementMessageRefCount(respMsg);
			return;
		}

		// our response is ready to go, send it!
		if( !cxa_rpc_protocolParser_writeMessage(&nrIn->protocolParser, respMsg) )
		{
			cxa_logger_warn(&nrIn->logger, "error writing name response");
			cxa_rpc_messageFactory_decrementMessageRefCount(respMsg);
			return;
		}

		cxa_rpc_messageFactory_decrementMessageRefCount(respMsg);
		cxa_logger_debug(&nrIn->logger, "sent name response");

		// notify our listeners
		cxa_array_iterate(&nrIn->linkListeners, currListener, cxa_rpc_nodeRemote_linkListener_t)
		{
			if( currListener == NULL ) continue;
			if( currListener->cb_linkEstablished != NULL ) currListener->cb_linkEstablished(nrIn, currListener->userVar);
		}
	}
	else cxa_logger_warn(&nrIn->logger, "unknown downstream linkManagement message received");
}
