/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_rpc_nodeRemote.h"


// ******** includes ********
#include <stdio.h>
#include <string.h>
#include <cxa_assert.h>
#include <cxa_rpc_node.h>
#include <cxa_rpc_messageFactory.h>
#include <cxa_stringUtils.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define PROTO_VERSION						2
#define PROVISION_TIMEOUT_MS				5000
#define LINK_MANAGEMENT_DEST				"_linkMan"
#define LINK_MANAGEMENT_METHOD_PROVISON		"_getName"
#define LINK_MANAGEMENT_ID_PROVISON			1234


// ******** local type definitions ********


// ******** local function prototypes ********
static bool isUpstream(cxa_rpc_nodeRemote_t *const nrIn);

static void handleMessage_upstream(cxa_rpc_messageHandler_t *const handlerIn, cxa_rpc_message_t *const msgIn);
static bool handleMessage_downstream(cxa_rpc_messageHandler_t *const handlerIn, cxa_rpc_message_t *const msgIn);

static void packetReceived_cb(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn);

static void handleLinkManagement_upstream(cxa_rpc_nodeRemote_t *const nrIn, cxa_rpc_message_t *const msgIn);
static void handleLinkManagement_downstream(cxa_rpc_nodeRemote_t *const nrIn, cxa_rpc_message_t *const msgIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_rpc_nodeRemote_init_upstream(cxa_rpc_nodeRemote_t *const nrIn, cxa_ioStream_t *const ioStreamIn, cxa_timeBase_t *const timeBaseIn)
{
	cxa_assert(nrIn);
	cxa_assert(ioStreamIn);

	// initialize our super class (but we don't have a name yet)
	cxa_rpc_messageHandler_init(&nrIn->super, handleMessage_upstream, handleMessage_downstream);

	// setup our initial state
	nrIn->downstreamSubNode = NULL;
	nrIn->isProvisioned = false;
	cxa_array_initStd(&nrIn->linkListeners, nrIn->linkListeners_raw);

	// setup our logger
	cxa_logger_vinit(&nrIn->super.logger, "rpcNr_us_%p", nrIn);

	// get a message (and buffer) for our protocol parser
	cxa_rpc_message_t* rxMsg = cxa_rpc_messageFactory_getFreeMessage_empty();
	cxa_assert(rxMsg);

	// initialize our protocol parser
	cxa_protocolParser_init(&nrIn->protocolParser, ioStreamIn, cxa_rpc_message_getBuffer(rxMsg), timeBaseIn);
	cxa_protocolParser_addPacketListener(&nrIn->protocolParser, packetReceived_cb, nrIn);
}


bool cxa_rpc_nodeRemote_init_downstream(cxa_rpc_nodeRemote_t *const nrIn, cxa_ioStream_t *const ioStreamIn, cxa_timeBase_t *const timeBaseIn, cxa_rpc_node_t *const subNodeIn)
{
	cxa_assert(nrIn);
	cxa_assert(ioStreamIn);
	cxa_assert(timeBaseIn);

	// initialize our super class (but we we won't use our name)
	cxa_rpc_messageHandler_init(&nrIn->super, handleMessage_upstream, handleMessage_downstream);

	// setup our initial state
	nrIn->downstreamSubNode = subNodeIn;
	nrIn->isProvisioned = false;
	cxa_timeDiff_init(&nrIn->td_provision, timeBaseIn, false);
	cxa_array_initStd(&nrIn->linkListeners, nrIn->linkListeners_raw);

	// setup our logger
	cxa_logger_vinit(&nrIn->super.logger, "rpcNr_ds_%s", cxa_rpc_node_getName(subNodeIn));

	// get a message (and buffer) for our protocol parser
	cxa_rpc_message_t* rxMsg = cxa_rpc_messageFactory_getFreeMessage_empty();
	cxa_assert(rxMsg);

	// initialize our protocol parser
	cxa_protocolParser_init(&nrIn->protocolParser, ioStreamIn, cxa_rpc_message_getBuffer(rxMsg), timeBaseIn);
	cxa_protocolParser_addPacketListener(&nrIn->protocolParser, packetReceived_cb, nrIn);

	// set ourselves as the downstream node's parent
	if( subNodeIn->super.parent != NULL )
	{
		cxa_logger_warn(&nrIn->super.logger, "attempted downstream node %p already has parent", subNodeIn);
		return false;
	}

	// make sure we're not trying to add the global root as a subnode
	if( subNodeIn->isGlobalRoot )
	{
		cxa_logger_warn(&nrIn->super.logger, "cannot add the global root as downstream subnode");
		return false;
	}

	// if we made it here, we're good to add
	subNodeIn->super.parent = &nrIn->super;
	subNodeIn->isLocalRoot = true;

	cxa_logger_debug(&nrIn->super.logger, "owns node '%s' @ [%p]", cxa_rpc_node_getName(subNodeIn), subNodeIn);
	return true;
}


void cxa_rpc_nodeRemote_deinit(cxa_rpc_nodeRemote_t *const nrIn)
{
	cxa_assert(nrIn);

	cxa_rpc_message_t* rxMsg = cxa_rpc_messageFactory_getMessage_byBuffer(cxa_protocolParser_getBuffer(&nrIn->protocolParser));
	if( rxMsg != NULL )
	{
		cxa_protocolParser_setBuffer(&nrIn->protocolParser, NULL);

		if( cxa_rpc_messageFactory_getReferenceCountForMessage(rxMsg) > 0 ) cxa_rpc_messageFactory_decrementMessageRefCount(rxMsg);
	}
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

	// see if we need to try and provision ourselves...
	if( !isUpstream(nrIn) && !nrIn->isProvisioned && cxa_timeDiff_isElapsed_recurring_ms(&nrIn->td_provision, PROVISION_TIMEOUT_MS) && (nrIn->downstreamSubNode != NULL) )
	{
		cxa_rpc_message_t* nameReqMsg = cxa_rpc_messageFactory_getFreeMessage_empty();
		cxa_linkedField_t* params = NULL;
		if( !nameReqMsg ||
				!cxa_rpc_message_initRequest(nameReqMsg, LINK_MANAGEMENT_DEST, LINK_MANAGEMENT_METHOD_PROVISON, NULL, 0) ||
				!cxa_rpc_message_setId(nameReqMsg, LINK_MANAGEMENT_ID_PROVISON) ||
				!cxa_rpc_message_prependNodeNameToSource(nameReqMsg, LINK_MANAGEMENT_DEST) ||
				((params = cxa_rpc_message_getParams(nameReqMsg)) == NULL) ||
				!cxa_linkedField_append_cString(params, cxa_rpc_node_getName(nrIn->downstreamSubNode)) )
		{
			cxa_logger_warn(&nrIn->super.logger, "error getting/initializing provisioning request, will retry");
			cxa_rpc_messageFactory_decrementMessageRefCount(nameReqMsg);
			return;
		}

		if( !cxa_protocolParser_writePacket(&nrIn->protocolParser, cxa_rpc_message_getBuffer(nameReqMsg)) )
		{
			cxa_logger_warn(&nrIn->super.logger, "error writing provisioning request, will retry");
			cxa_rpc_messageFactory_decrementMessageRefCount(nameReqMsg);
			return;
		}

		cxa_logger_debug(&nrIn->super.logger, "sent provision request");
		cxa_rpc_messageFactory_decrementMessageRefCount(nameReqMsg);
	}

	cxa_protocolParser_update(&nrIn->protocolParser);
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
		cxa_logger_warn(&nrIn->super.logger, "handleUpstream(%p): called on upstream side, dropping message", msgIn);
		return;
	}

	// we have a message that we need to send via our ioStream
	if( !cxa_protocolParser_writePacket(&nrIn->protocolParser, cxa_rpc_message_getBuffer(msgIn)) )
	{
		cxa_logger_warn(&nrIn->super.logger, "handleUpstream(%p): protocolParser reports write error, dropping message", msgIn);
		return;
	}
}


static bool handleMessage_downstream(cxa_rpc_messageHandler_t *const handlerIn, cxa_rpc_message_t *const msgIn)
{
	cxa_assert(handlerIn);
	cxa_rpc_nodeRemote_t* nrIn = (cxa_rpc_nodeRemote_t*)handlerIn;
	if( !msgIn ) return false;

	// this function should only be called on the upstream side of the connection
	if( !isUpstream(nrIn) )
	{
		cxa_logger_warn(&nrIn->super.logger, "handleDownstream(%p): called on downstream side, dropping message", msgIn);
		return false;
	}

	// we have a message that we need to send via our ioStream
	if( !cxa_protocolParser_writePacket(&nrIn->protocolParser, cxa_rpc_message_getBuffer(msgIn)) )
	{
		cxa_logger_warn(&nrIn->super.logger, "handleDownstream(%p): protocolParser reports write error, dropping message", msgIn);
		return false;
	}

	return true;
}


static void packetReceived_cb(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn)
{
	cxa_assert(packetIn);
	cxa_assert(userVarIn);

	cxa_rpc_nodeRemote_t* nrIn = (cxa_rpc_nodeRemote_t*)userVarIn;

	// verify that this is actually an rpc message
	cxa_rpc_message_t* rxMsg = cxa_rpc_messageFactory_getMessage_byBuffer(packetIn);
	if( rxMsg == NULL)
	{
		cxa_logger_warn(&nrIn->super.logger, "received message using unknown message buffer, dropping");
		return;
	}

	// if we made it here, we should validate the message
	if( !cxa_rpc_message_validateReceivedBytes(rxMsg) )
	{
		cxa_logger_debug(&nrIn->super.logger, "invalid message received");
		return;
	}
	cxa_logger_trace(&nrIn->super.logger, "message validated successfully");


	// check for our exclusive link management messages...
	char* dest = cxa_rpc_message_getDestination(rxMsg);
	if( (dest != NULL) && cxa_stringUtils_equals(dest, LINK_MANAGEMENT_DEST))
	{
		// this is a link management message...process locally and discard
		if( isUpstream(nrIn) )
		{
			handleLinkManagement_upstream(nrIn, rxMsg);
		}
		else
		{
			handleLinkManagement_downstream(nrIn, rxMsg);
		}
	}
	else
	{
		// normal operation...depends what side we are on...
		if( isUpstream(nrIn) )
		{
			// pass to our parent for processing
			cxa_rpc_messageHandler_handleUpstream(nrIn->super.parent, rxMsg);
		}
		else
		{
			// pass to our subNode for processing
			cxa_rpc_messageHandler_handleDownstream(&nrIn->downstreamSubNode->super, rxMsg);
		}
	}


	// we need to check our reference count for our message and get a new one
	// (if one of our callbacks is still using this rxMsg)
	uint8_t refCount = cxa_rpc_messageFactory_getReferenceCountForMessage(rxMsg);
	// make sure there wasn't some weirdness happening
	if( refCount == 0 )
	{
		cxa_logger_warn(&nrIn->super.logger, "over-freed rx buffer (%p)", rxMsg);
		// this was a warning, but we _should_ still be able to use this message
		cxa_rpc_message_resetForRx(rxMsg);
	}
	else if( refCount == 1 )
	{
		// we need to reset our current buffer for re-use
		cxa_rpc_message_resetForRx(rxMsg);
	}
	else
	{
		// we need to reserve another buffer (this one is still being used)
		cxa_logger_trace(&nrIn->super.logger, "message is still in-use, requesting new rx buffer");

		// release _our_ lock on the message
		cxa_rpc_messageFactory_decrementMessageRefCount(rxMsg);

		// get a new message
		cxa_rpc_message_t* rxMsg = cxa_rpc_messageFactory_getFreeMessage_empty();
		if( rxMsg == NULL ) cxa_logger_warn(&nrIn->super.logger, "could not reserve free rx buffer");

		// if we didn't get an rx buffer, protocol parser will become idle automatically
		cxa_protocolParser_setBuffer(&nrIn->protocolParser, (rxMsg != NULL)? cxa_rpc_message_getBuffer(rxMsg) : NULL);
	}

}


static void handleLinkManagement_upstream(cxa_rpc_nodeRemote_t *const nrIn, cxa_rpc_message_t *const msgIn)
{
	cxa_assert(nrIn);
	cxa_assert(msgIn);

	char *method = NULL;
	if( (cxa_rpc_message_getType(msgIn) == CXA_RPC_MESSAGE_TYPE_REQUEST) &&
			((method = cxa_rpc_message_getMethod(msgIn)) != NULL) && cxa_stringUtils_equals(method, LINK_MANAGEMENT_METHOD_PROVISON) )
	{
		// this is a request to provision

		cxa_linkedField_t* params = cxa_rpc_message_getParams(msgIn);
		if( params == NULL )
		{
			cxa_logger_warn(&nrIn->super.logger, "no params for provision request");
			return;
		}
		char* nodeName = (char*)cxa_linkedField_get_pointerToIndex(params, 0);
		if( (nodeName == NULL) || (strlen(nodeName) == 0) )
		{
			cxa_logger_warn(&nrIn->super.logger, "provisioning name is length 0");
			return;
		}

		//@TODO perform authentication here
		cxa_logger_debug(&nrIn->super.logger, "provision request from subnode '%s'", nodeName);

		// get our response ready
		cxa_rpc_message_t* respMsg = cxa_rpc_messageFactory_getFreeMessage_empty();
		cxa_linkedField_t* retParams;
		if( (respMsg == NULL) ||
				!cxa_rpc_message_initResponse(respMsg, LINK_MANAGEMENT_DEST, cxa_rpc_message_getId(msgIn), CXA_RPC_METHOD_RETVAL_SUCCESS) ||
				!cxa_rpc_message_prependNodeNameToSource(respMsg, LINK_MANAGEMENT_DEST) ||
				((retParams = cxa_rpc_message_getParams(respMsg)) == NULL) ||
				!cxa_linkedField_append_cString(retParams, nodeName) )
		{
			cxa_logger_warn(&nrIn->super.logger, "error getting/initializing provisioning response");
			cxa_rpc_messageFactory_decrementMessageRefCount(respMsg);
			return;
		}

		// our response is ready to go, send it!
		if( !cxa_protocolParser_writePacket(&nrIn->protocolParser, cxa_rpc_message_getBuffer(respMsg)) )
		{
			cxa_logger_warn(&nrIn->super.logger, "error writing provision response");
			cxa_rpc_messageFactory_decrementMessageRefCount(respMsg);
			return;
		}

		cxa_rpc_messageFactory_decrementMessageRefCount(respMsg);
		cxa_logger_debug(&nrIn->super.logger, "sent provision response");

		// notify our listeners
		cxa_array_iterate(&nrIn->linkListeners, currListener, cxa_rpc_nodeRemote_linkListener_t)
		{
			if( currListener == NULL ) continue;
			if( currListener->cb_linkEstablished != NULL ) currListener->cb_linkEstablished(nrIn, currListener->userVar);
		}
	}
	else cxa_logger_warn(&nrIn->super.logger, "unknown upstream linkManagement message received");
}


static void handleLinkManagement_downstream(cxa_rpc_nodeRemote_t *const nrIn, cxa_rpc_message_t *const msgIn)
{
	cxa_assert(nrIn);
	cxa_assert(msgIn);

	cxa_linkedField_t* retParams;
	char* respName;

	if( (cxa_rpc_message_getType(msgIn) == CXA_RPC_MESSAGE_TYPE_RESPONSE) &&
			(cxa_rpc_message_getId(msgIn) == LINK_MANAGEMENT_ID_PROVISON) &&
			((retParams = cxa_rpc_message_getParams(msgIn)) != NULL) &&
			cxa_linkedField_get_cstring_inPlace(retParams, 0, &respName, NULL) )
	{
		// we have a properly formatted provision response...
		// check out the name and make sure it's for us
		if( !cxa_stringUtils_equals(respName, cxa_rpc_node_getName(nrIn->downstreamSubNode)) )
		{
			// not for us
			cxa_logger_debug(&nrIn->super.logger, "provision response received, not us");
			return;
		}

		// this a provisioning response for us...check the return value
		if( !cxa_rpc_message_getReturnValue(msgIn) )
		{
			nrIn->isProvisioned = false;
			cxa_logger_warn(&nrIn->super.logger, "provision request denied");
			return;
		}

		nrIn->isProvisioned = true;
		cxa_logger_info(&nrIn->super.logger, "provisioned successfully");

		// notify our listeners
		cxa_array_iterate(&nrIn->linkListeners, currListener, cxa_rpc_nodeRemote_linkListener_t)
		{
			if( currListener == NULL ) continue;
			if( currListener->cb_linkEstablished != NULL ) currListener->cb_linkEstablished(nrIn, currListener->userVar);
		}
	}
	else cxa_logger_warn(&nrIn->super.logger, "unknown/invalid downstream linkManagement message received");
}
