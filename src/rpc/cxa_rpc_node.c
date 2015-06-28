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
#include "cxa_rpc_node.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <string.h>
#include <cxa_assert.h>
#include <cxa_rpc_messageFactory.h>
#include <cxa_timeDiff.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void cxa_rpc_node_commonInit(cxa_rpc_node_t *const nodeIn, char *const nameIn, cxa_timeBase_t *const timeBaseIn, bool isGlobalRootIn);
static void handleMessage_upstream(cxa_rpc_node_t *const nodeIn, cxa_rpc_message_t *const msgIn);
static void handleMessage_downstream(cxa_rpc_node_t *const nodeIn, cxa_rpc_message_t *const msgIn);
static void handleMessage_atDestination(cxa_rpc_node_t *const nodeIn, cxa_rpc_message_t *const msgIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_rpc_node_init(cxa_rpc_node_t *const nodeIn, char *const nameIn, cxa_timeBase_t *const timeBaseIn)
{
	cxa_rpc_node_commonInit(nodeIn, nameIn, timeBaseIn, false);
}


void cxa_rpc_node_init_globalRoot(cxa_rpc_node_t *const nodeIn, cxa_timeBase_t *const timeBaseIn)
{
	cxa_rpc_node_commonInit(nodeIn, CXA_RPC_PATH_GLOBAL_ROOT, timeBaseIn, true);
}


bool cxa_rpc_node_addSubNode(cxa_rpc_node_t *const nodeIn, cxa_rpc_node_t *const subNodeIn)
{
	cxa_assert(nodeIn);
	cxa_assert(subNodeIn);

	if( subNodeIn->parent != NULL )
	{
		cxa_logger_warn(&nodeIn->logger, "attempted subnode %p already has parent", subNodeIn);
		return false;
	}

	// make sure we're not trying to add the global root as a subnode
	if( subNodeIn->isGlobalRoot )
	{
		cxa_logger_warn(&nodeIn->logger, "cannot add the global root as a subnode");
		return false;
	}

	// if we made it here, we're good to add
	if( !cxa_array_append(&nodeIn->subnodes, (void*)&subNodeIn) ) return false;
	subNodeIn->parent = nodeIn;

	cxa_logger_debug(&nodeIn->logger, "owns node '%s' @ [%p]", subNodeIn->name, subNodeIn);

	return true;
}


bool cxa_rpc_node_addMethod(cxa_rpc_node_t *const nodeIn, char *const nameIn, cxa_rpc_node_method_cb_t cbIn, void* userVarIn)
{
	cxa_assert(nodeIn);
	cxa_assert(nameIn);

	// create our new entry
	cxa_rpc_node_method_cbEntry_t newEntry = {.cb=cbIn, .userVar=userVarIn};

	// save our name (and make sure it's null-terminated)
	strncpy(newEntry.name, nameIn, CXA_RPC_NODE_MAX_METHOD_NAME_LEN_BYTES);
	newEntry.name[CXA_RPC_NODE_MAX_METHOD_NAME_LEN_BYTES] = 0;

	// add to our methods
	return cxa_array_append(&nodeIn->methods, &newEntry);
}


void cxa_rpc_node_sendMessage_async(cxa_rpc_node_t *const nodeIn, cxa_rpc_message_t* const msgIn)
{
	cxa_assert(nodeIn);
	cxa_assert(msgIn);

	switch( cxa_rpc_message_getType(msgIn) )
	{
		case CXA_RPC_MESSAGE_TYPE_REQUEST:
			cxa_logger_debug(&nodeIn->logger, "sending request with id %lu", cxa_rpc_message_getId(msgIn));
			break;

		case CXA_RPC_MESSAGE_TYPE_RESPONSE:
			cxa_logger_debug(&nodeIn->logger, "sending response with id %lu", cxa_rpc_message_getId(msgIn));
			break;

		default:
			cxa_logger_warn(&nodeIn->logger, "refusing to send message with unknown type");
			return;
	}

	handleMessage_upstream(nodeIn, msgIn);
}


cxa_rpc_message_t* cxa_rpc_node_sendRequest_sync(cxa_rpc_node_t *const nodeIn, cxa_rpc_message_t* const msgIn, uint32_t timeOut_msIn)
{
	cxa_assert(nodeIn);
	cxa_assert(msgIn);

	cxa_rpc_message_type_t msgType = cxa_rpc_message_getType(msgIn);
	if( msgType != CXA_RPC_MESSAGE_TYPE_REQUEST ) return NULL;

	// set the ID
	if( !cxa_rpc_message_setId(msgIn, nodeIn->currId) ) return false;
	nodeIn->currId = (nodeIn->currId == CXA_RPC_ID_MAX) ? 1 : nodeIn->currId+1;

	// create a new inflight request entry and add to our list
	cxa_rpc_node_inflightSyncRequestEntry_t *newEntry = cxa_array_append_empty(&nodeIn->inflightSyncRequests);
	if( newEntry == NULL) return NULL;
	newEntry->response = NULL;
	newEntry->id=cxa_rpc_message_getId(msgIn);

	// send our request
	cxa_timeDiff_t td_resp;
	cxa_timeDiff_init(&td_resp, nodeIn->timeBase);
	cxa_rpc_node_sendMessage_async(nodeIn, msgIn);

	// wait for the response
	while( newEntry->response == NULL )
	{
		// see if we've timed out
		if( cxa_timeDiff_isElapsed_ms(&td_resp, timeOut_msIn) )
		{
			if( !cxa_array_remove(&nodeIn->inflightSyncRequests, newEntry) )
			{
				cxa_logger_warn(&nodeIn->logger, "error removing inflightSyncReq after timeout");
			}
			cxa_logger_debug(&nodeIn->logger, "request id %lu timed out", cxa_rpc_message_getId(msgIn));
			return NULL;
		}
	}

	cxa_logger_debug(&nodeIn->logger, "sync transaction id %lu complete", cxa_rpc_message_getId(msgIn));

	// if we made it here, we have a response!
	cxa_rpc_message_t* retVal = newEntry->response;
	if( !cxa_array_remove(&nodeIn->inflightSyncRequests, newEntry) )
	{
		cxa_logger_warn(&nodeIn->logger, "error removing inflightSyncReq after rx");
	}

	return retVal;
}


// ******** local function implementations ********
static void cxa_rpc_node_commonInit(cxa_rpc_node_t *const nodeIn, char *const nameIn, cxa_timeBase_t *const timeBaseIn, bool isGlobalRootIn)
{
	cxa_assert(nodeIn);
	cxa_assert(timeBaseIn);
	cxa_assert(nameIn);

	// save our name (and make sure it's null-terminated)
	strncpy(nodeIn->name, nameIn, CXA_RPC_NODE_MAX_NAME_LEN_BYTES);
	nodeIn->name[CXA_RPC_NODE_MAX_NAME_LEN_BYTES] = 0;

	// setup our initial state (global roots, by default, are also local roots)
	nodeIn->parent = NULL;
	nodeIn->timeBase = timeBaseIn;
	nodeIn->isGlobalRoot = isGlobalRootIn;
	nodeIn->isLocalRoot = isGlobalRootIn;
	nodeIn->currId = 1;
	cxa_array_initStd(&nodeIn->subnodes, nodeIn->subnodes_raw);
	cxa_array_initStd(&nodeIn->methods, nodeIn->methods_raw);
	cxa_array_initStd(&nodeIn->inflightSyncRequests, nodeIn->inflightSyncRequests_raw);

	// setup our logger
	cxa_logger_vinit(&nodeIn->logger, "rpcNode_%s", nodeIn->name);
}


static void handleMessage_upstream(cxa_rpc_node_t *const nodeIn, cxa_rpc_message_t *const msgIn)
{
	cxa_assert(nodeIn);
	if( !msgIn ) return;

	// get the first path component but don't strip it
	char* pathComp = NULL;
	size_t pathCompLen_bytes = 0;
	if( !cxa_rpc_message_destination_getFirstPathComponent(msgIn, &pathComp, &pathCompLen_bytes) ||
			(pathComp == NULL) || (pathCompLen_bytes == 0) )
	{
		cxa_logger_trace(&nodeIn->logger, "handleUpstream(%p): unable to parse path component, dropping message", msgIn);
		return;
	}

	cxa_logger_debug(&nodeIn->logger, "handleUpstream(%p): '%s'", msgIn, pathComp);

	// prepend ourselves
	cxa_rpc_message_prependNodeNameToSource(msgIn, nodeIn->name);

	// depends upon the path component
	if( strncmp(pathComp, CXA_RPC_PATH_UP_ONE_LEVEL, strlen(CXA_RPC_PATH_UP_ONE_LEVEL)) == 0 )
	{
		// ../nodeX  --  strip UP_ONE_LEVEL and pass to our parent, then bail
		if( !cxa_rpc_message_destination_removeFirstPathComponent(msgIn) ) return;
		if( nodeIn->parent != NULL ) handleMessage_upstream(nodeIn->parent, msgIn);
		return;
	}
	else if( strncmp(pathComp, CXA_RPC_PATH_GLOBAL_ROOT, strlen(CXA_RPC_PATH_GLOBAL_ROOT)) == 0 )
	{
		// /nodeX  --  looking for global root...if we're not it, pass it up
		if( !nodeIn->isGlobalRoot )
		{
			// we are not global root...pass it up, then bail
			if( nodeIn->parent != NULL ) handleMessage_upstream(nodeIn->parent, msgIn);
			return;
		}

		// if we made it here, we _are_ global root, we need to strip our delimiter,
		// get the local node name, and figure out what to do
		if( !cxa_rpc_message_destination_removeFirstPathComponent(msgIn) ) return;
		if( !cxa_rpc_message_destination_getFirstPathComponent(msgIn, &pathComp, &pathCompLen_bytes) ||
					(pathComp == NULL) || (pathCompLen_bytes == 0) )
		{
			// there's no more destination components...this must have been meant for us...
			cxa_logger_debug(&nodeIn->logger, "handleUpstream(%p): msg reached dest, processing", msgIn);
			handleMessage_atDestination(nodeIn, msgIn);
			return;
		}

		// we got a local node name...process below
	}
	else if( strncmp(pathComp, CXA_RPC_PATH_LOCAL_ROOT, strlen(CXA_RPC_PATH_LOCAL_ROOT)) == 0 )
	{
		// ~nodeX  --  looking for local root...if we're not it, pass it up
		if( !nodeIn->isLocalRoot )
		{
			// we are not local root...pass it up, then bail
			handleMessage_upstream(nodeIn->parent, msgIn);
			return;
		}

		// if we made it here, we _are_ local root, we need to strip our delimiter,
		// get the local node name, and figure out what to do
		if( !cxa_rpc_message_destination_removeFirstPathComponent(msgIn) ) return;
		if( !cxa_rpc_message_destination_getFirstPathComponent(msgIn, &pathComp, &pathCompLen_bytes) ||
					(pathComp == NULL) || (pathCompLen_bytes == 0) )
		{
			// there's no more destination components...this must have been meant for us...
			cxa_logger_debug(&nodeIn->logger, "handleUpstream(%p): msg reached dest, processing", msgIn);
			handleMessage_atDestination(nodeIn, msgIn);
			return;
		}

		// we got a local node name...process below
	}

	// if we made it here, we need to process a local nodeName...
	cxa_array_iterate(&nodeIn->subnodes, currSubNode, cxa_rpc_node_t*)
	{
		if( currSubNode == NULL) continue;

		if( strncmp(pathComp, (*currSubNode)->name, pathCompLen_bytes) == 0 )
		{
			// we have a match for the node name...start going down stream
			if( !cxa_rpc_message_destination_removeFirstPathComponent(msgIn) ) return;
			handleMessage_downstream(*currSubNode, msgIn);
			return;
		}
	}

	// if we made it here, we failed
	cxa_logger_trace(&nodeIn->logger, "handleUpstream(%p): unable to route '%s'", msgIn, pathComp);
}


static void handleMessage_downstream(cxa_rpc_node_t *const nodeIn, cxa_rpc_message_t *const msgIn)
{
	cxa_assert(nodeIn);
	cxa_assert(msgIn);

	// get the first path component and strip it
	char* pathComp = NULL;
	size_t pathCompLen_bytes = 0;
	if( !cxa_rpc_message_destination_getFirstPathComponent(msgIn, &pathComp, &pathCompLen_bytes) ||
				(pathComp == NULL) || (pathCompLen_bytes == 0) )
	{
		// couldn't parse a path component...must have been meant for us
		cxa_logger_debug(&nodeIn->logger, "handleDownstream(%p): msg reached dest, processing", msgIn);
		handleMessage_atDestination(nodeIn, msgIn);
		return;
	}

	cxa_logger_debug(&nodeIn->logger, "handleDownstream(%p): '%s'", msgIn, pathComp);

	// if we made it here, it wasn't meant for us...it was meant for a subnode

	// process a local node name
	cxa_array_iterate(&nodeIn->subnodes, currSubNode, cxa_rpc_node_t*)
	{
		if( currSubNode == NULL) continue;

		if( strcmp(pathComp, (*currSubNode)->name) == 0 )
		{
			// we have a match for the node name...keep going down stream
			if( !cxa_rpc_message_destination_removeFirstPathComponent(msgIn) ) return;
			handleMessage_downstream(*currSubNode, msgIn);
			return;
		}
	}

	// if we made it here, we failed
	cxa_logger_trace(&nodeIn->logger, "handleDownStream(%p): unable to route '%s'", msgIn, pathComp);
}


static void handleMessage_atDestination(cxa_rpc_node_t *const nodeIn, cxa_rpc_message_t *const msgIn)
{
	cxa_assert(nodeIn);
	cxa_assert(msgIn);

	switch( cxa_rpc_message_getType(msgIn) )
	{
		case CXA_RPC_MESSAGE_TYPE_REQUEST:
		{
			char* methodNameIn = cxa_rpc_message_getMethod(msgIn);
			if( methodNameIn == NULL)
			{
				cxa_logger_trace(&nodeIn->logger, "atDest(%p): unable to parse method, dropping message", msgIn);
				return;
			}

			cxa_array_iterate(&nodeIn->methods, currMethod, cxa_rpc_node_method_cbEntry_t)
			{
				if( strcmp(methodNameIn, currMethod->name) == 0 )
				{
					cxa_logger_trace(&nodeIn->logger, "atDest(%p): found method '%s'", msgIn, methodNameIn);

					// we found the method...setup our response message
					cxa_rpc_message_t* respMsg = cxa_rpc_messageFactory_getFreeMessage_empty();
					if( !cxa_rpc_message_initResponse(respMsg, cxa_rpc_message_getSource(msgIn), cxa_rpc_message_getId(msgIn)))
					{
						cxa_logger_warn(&nodeIn->logger, "atDest(%p): error initializing response", msgIn);
						cxa_rpc_messageFactory_decrementMessageRefCount(respMsg);
						return;
					}

					// only execute if we have a callback
					if( currMethod->cb != NULL)
					{
						cxa_logger_debug(&nodeIn->logger, "atDest(%p): executing method '%s'", msgIn, methodNameIn);
						if( currMethod->cb(cxa_rpc_message_getParams(msgIn), cxa_rpc_message_getParams(respMsg), currMethod->userVar) )
						{
							// we need to send the response
							cxa_logger_debug(&nodeIn->logger, "atDest(%p): sending response %p", msgIn, respMsg);
							cxa_rpc_node_sendMessage_async(nodeIn, respMsg);
						}
					}

					// free our hold on the response
					cxa_rpc_messageFactory_decrementMessageRefCount(respMsg);
					return;
				}
			}

			// if we made it here, we failed
			cxa_logger_trace(&nodeIn->logger, "atDest(%p): unknown method '%s', dropping message", msgIn, methodNameIn);
			return;
		}

		case CXA_RPC_MESSAGE_TYPE_RESPONSE:
		{
			CXA_RPC_ID_DATATYPE respId = cxa_rpc_message_getId(msgIn);
			cxa_logger_debug(&nodeIn->logger, "atDest(%p): received response id %d", msgIn, respId);

			// look through our inflight requests to see if we have anything outstanding
			cxa_array_iterate(&nodeIn->inflightSyncRequests, currReq, cxa_rpc_node_inflightSyncRequestEntry_t)
			{
				if( currReq->id == respId )
				{
					// we found our request...save off this response
					cxa_logger_debug(&nodeIn->logger, "atDest(%p): found sync tranaction for id %d", msgIn, respId);
					currReq->response = msgIn;
					return;
				}
			}

			cxa_logger_trace(&nodeIn->logger, "atDest(%p): no sync transcation found for id %d, dropping message", msgIn, respId);

			break;
		}

		default: break;
	}
}
