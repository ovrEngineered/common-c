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
#include "cxa_mqtt_rpc_node.h"


// ******** includes ********
#include <string.h>
#include <cxa_assert.h>
#include <cxa_mqtt_messageFactory.h>
#include <cxa_mqtt_message_publish.h>
#include <cxa_mqtt_rpc_message.h>
#include <cxa_mqtt_rpc_node_root.h>
#include <cxa_stringUtils.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define REQUEST_TIMEOUT_MS			3000


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_handleMessage_upstream(cxa_mqtt_rpc_node_t *const superIn, cxa_mqtt_message_t *const msgIn);
static bool scm_handleMessage_downstream(cxa_mqtt_rpc_node_t *const superIn,
										 char *const remainingTopicIn, uint16_t remainingTopicLen_bytesIn,
										 cxa_mqtt_message_t *const msgIn);

static cxa_mqtt_message_t* prepForResponse(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_message_t* reqMsgIn, cxa_linkedField_t **lf_payloadIn, cxa_linkedField_t **lf_retPayloadIn);
static void sendResponse(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_methodRetVal_t retValIn, cxa_mqtt_message_t *responseMessageIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_mqtt_rpc_node_vinit(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn, const char *nameFmtIn, va_list varArgsIn)
{
	cxa_assert(nodeIn);
	cxa_assert(nameFmtIn);

	// save our references and set some defaults
	nodeIn->parentNode = parentNodeIn;
	nodeIn->scm_handleMessage_upstream = scm_handleMessage_upstream;
	nodeIn->scm_handleMessage_downstream = scm_handleMessage_downstream;

	// assemble our name
	cxa_assert(vsnprintf(nodeIn->name, CXA_MQTT_RPCNODE_MAXLEN_NAME_BYTES, nameFmtIn, varArgsIn) < CXA_MQTT_RPCNODE_MAXLEN_NAME_BYTES);
	nodeIn->name[CXA_MQTT_RPCNODE_MAXLEN_NAME_BYTES-1] = 0;

	// setup our subnodes, methods, outstanding requests
	cxa_array_initStd(&nodeIn->subNodes, nodeIn->subNodes_raw);
	cxa_array_initStd(&nodeIn->methods, nodeIn->methods_raw);
	cxa_array_initStd(&nodeIn->outstandingRequests, nodeIn->outstandingRequests_raw);

	// setup our logger
	cxa_logger_vinit(&nodeIn->logger, "mRpcNode_%s", nodeIn->name);

	// add as a subnode (if we have a parent)
	if( nodeIn->parentNode != NULL ) cxa_assert( cxa_array_append(&nodeIn->parentNode->subNodes, (void*)&nodeIn) );
}


void cxa_mqtt_rpc_node_addMethod(cxa_mqtt_rpc_node_t *const nodeIn, char *const nameIn, cxa_mqtt_rpc_cb_method_t cb_methodIn, void* userVarIn)
{
	cxa_assert(nodeIn);
	cxa_assert(cb_methodIn);

	cxa_mqtt_rpc_node_methodEntry_t newEntry = {
		.cb_method = cb_methodIn,
		.userVar = userVarIn
	};
	cxa_assert(nameIn && (strlen(nameIn) < (sizeof(newEntry.name)-1)) );
	strlcpy(newEntry.name, nameIn, sizeof(newEntry.name));
	cxa_assert( cxa_array_append(&nodeIn->methods, &newEntry) );
}


bool cxa_mqtt_rpc_node_executeMethod(cxa_mqtt_rpc_node_t *const nodeIn,
									 char *const methodNameIn, char *const pathToNodeIn, cxa_fixedByteBuffer_t *const paramsIn,
									 cxa_mqtt_rpc_cb_methodResponse_t responseCbIn, void* userVarIn)
{
	cxa_assert(nodeIn);
	cxa_assert(pathToNodeIn);
	cxa_assert(methodNameIn);

	static uint16_t currRequestId = 0;

	// first, we need to form our message
	cxa_mqtt_message_t* msg = cxa_mqtt_messageFactory_getFreeMessage_empty();
	if( (msg == NULL) ||
		!cxa_mqtt_message_publish_init(msg, false, CXA_MQTT_QOS_ATMOST_ONCE, false,
									  "", 0,
									  ((paramsIn != NULL) ? cxa_fixedByteBuffer_get_pointerToIndex(paramsIn, 0) : NULL),
									  ((paramsIn != NULL) ? cxa_fixedByteBuffer_getSize_bytes(paramsIn) : 0)) )
	{
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}

	// now we need to get our topic/path in order...first the request ID
	char msgId[5];
	uint16_t sentReqId = currRequestId++;
	snprintf(msgId, sizeof(msgId), "%04X", sentReqId);
	msgId[4] = 0;
	if( !cxa_mqtt_message_publish_topicName_prependCString(msg, msgId) ||
		!cxa_mqtt_message_publish_topicName_prependCString(msg, "/") )
	{
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}

	// now the method name
	if( (strlen(methodNameIn) >= CXA_MQTT_RPCNODE_MAXLEN_METHOD_BYTES) ||
		!cxa_mqtt_message_publish_topicName_prependCString(msg, methodNameIn) ||
		!cxa_mqtt_message_publish_topicName_prependCString(msg, CXA_MQTT_RPCNODE_REQ_PREFIX) ||
		!cxa_mqtt_message_publish_topicName_prependCString(msg, "/") )
	{
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}

	// now the path to the node
	if( !cxa_mqtt_message_publish_topicName_prependCString(msg, pathToNodeIn) )
	{
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}

	// make sure we can get the topic name for the message before we go further
	char* remainingTopic;
	uint16_t remainingTopicLen_bytes;
	if( !cxa_mqtt_message_publish_getTopicName(msg, &remainingTopic, &remainingTopicLen_bytes) )
	{
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}

	// good, now add an outstanding request entry for this message (if desired)
	if( responseCbIn != NULL )
	{
		cxa_mqtt_rpc_node_outstandingRequest_t newRequest = {
				.cb = responseCbIn,
				.userVar = userVarIn
		};

		// copy over the method name and the id
		strlcpy(newRequest.name, methodNameIn, sizeof(newRequest.name));
		strlcpy(newRequest.id, msgId, sizeof(newRequest.id));

		// initialize our timeout
		cxa_timeDiff_init(&newRequest.td_timeout, true);

		// add to our list of outstanding requests
		if( !cxa_array_append(&nodeIn->outstandingRequests, &newRequest) )
		{
			cxa_logger_warn(&nodeIn->logger, "too many outstanding requests, dropping");
			cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
			return false;
		}
	}

	// excellent...now we need to figure out where this message is headed...
	if( cxa_stringUtils_startsWith(pathToNodeIn, "/") ||
		cxa_stringUtils_startsWith(pathToNodeIn, "~/"))
	{
		// this message is addressed from the global root or the
		// local root respectively...send it up!
		nodeIn->scm_handleMessage_upstream(nodeIn, msg);
	}
	else
	{
		// this message is addressed locally...therefore it must be headed
		// for one of _our_ subnodes...send it down!
		nodeIn->scm_handleMessage_downstream(nodeIn, remainingTopic, remainingTopicLen_bytes, msg);
	}

	// release our sent message
	cxa_mqtt_messageFactory_decrementMessageRefCount(msg);

	return true;
}


bool cxa_mqtt_rpc_node_publishNotification(cxa_mqtt_rpc_node_t *const nodeIn, char *const notiNameIn, cxa_mqtt_qosLevel_t qosIn, void* dataIn, size_t dataSize_bytesIn)
{
	cxa_assert(nodeIn);
	cxa_assert(notiNameIn);

	return false;
}


void cxa_mqtt_rpc_node_update(cxa_mqtt_rpc_node_t *const nodeIn)
{
	cxa_assert(nodeIn);

	// check our outstanding requests for timeouts
	cxa_array_iterate(&nodeIn->outstandingRequests, currRequest, cxa_mqtt_rpc_node_outstandingRequest_t)
	{
		if( currRequest == NULL ) continue;

		if( cxa_timeDiff_isElapsed_ms(&currRequest->td_timeout, REQUEST_TIMEOUT_MS) )
		{
			currRequest->cb(nodeIn, CXA_MQTT_RPC_METHODRETVAL_FAIL_TIMEOUT, NULL, currRequest->userVar);
			cxa_array_remove(&nodeIn->outstandingRequests, currRequest);
			break;
		}
	}

	// iterate through our subnodes and update them as well
	cxa_array_iterate(&nodeIn->subNodes, currSubNode, cxa_mqtt_rpc_node_t*)
	{
		if( currSubNode == NULL ) continue;
		cxa_mqtt_rpc_node_update(*currSubNode);
	}
}


// ******** local function implementations ********
static void scm_handleMessage_upstream(cxa_mqtt_rpc_node_t *const superIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(superIn);
	cxa_assert(msgIn);

	char* topicName;
	uint16_t topicNameLen_bytes;
	if( !cxa_mqtt_message_publish_getTopicName(msgIn, &topicName, &topicNameLen_bytes) ) return;

	cxa_logger_log_untermString(&superIn->logger, CXA_LOG_LEVEL_TRACE, "<< '", topicName, topicNameLen_bytes, "'");

	if( cxa_stringUtils_startsWith_withLengths(topicName, topicNameLen_bytes, "/", 1) ||
		cxa_stringUtils_startsWith_withLengths(topicName, topicNameLen_bytes, CXA_MQTT_RPCNODE_LOCALROOT_PREFIX, strlen(CXA_MQTT_RPCNODE_LOCALROOT_PREFIX)))
	{
		// this message is addressed from the global root or the local root respectively...send it up!
		if( superIn->parentNode != NULL ) superIn->parentNode->scm_handleMessage_upstream(superIn->parentNode, msgIn);
	}
	else
	{
		// this message is non-root relative...that means it probably originated someplace system-local
		// @TODO implement this
	}
}


static bool scm_handleMessage_downstream(cxa_mqtt_rpc_node_t *const superIn,
										 char *const remainingTopicIn, uint16_t remainingTopicLen_bytesIn,
										 cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(superIn);
	cxa_assert(remainingTopicIn);
	cxa_assert(msgIn);

	cxa_logger_log_untermString(&superIn->logger, CXA_LOG_LEVEL_TRACE, ">> '", remainingTopicIn, remainingTopicLen_bytesIn, "'");

	char *methodName, *id;
	size_t methodNameLen_bytes, idLen_bytes;

	// depends what type of message it is...
	if( cxa_mqtt_rpc_message_isActionableRequest(msgIn, NULL, NULL, NULL, NULL) )
	{
		// this is a request

		// make sure that the topic starts with our name (handle the special "localroot" case)
		size_t nodeNameLen_bytes = strlen(superIn->name);
		if( superIn->parentNode != NULL )
		{
			// not the local root node
			if( !cxa_stringUtils_startsWith_withLengths(remainingTopicIn, remainingTopicLen_bytesIn, superIn->name, nodeNameLen_bytes) ) return false;
		}
		else
		{
			// we are the local root node...check for name variants
			if( cxa_stringUtils_startsWith_withLengths(remainingTopicIn, remainingTopicLen_bytesIn, CXA_MQTT_RPCNODE_LOCALROOT_PREFIX, strlen(CXA_MQTT_RPCNODE_LOCALROOT_PREFIX)) )
			{
				nodeNameLen_bytes = strlen(CXA_MQTT_RPCNODE_LOCALROOT_PREFIX);
			}
			else if( !cxa_stringUtils_startsWith_withLengths(remainingTopicIn, remainingTopicLen_bytesIn, superIn->name, nodeNameLen_bytes) ) return false;
		}

		// so far so good...remove ourselves from the topic
		char* currTopic = remainingTopicIn + nodeNameLen_bytes;
		size_t currTopicLen_bytes = remainingTopicLen_bytesIn - nodeNameLen_bytes;

		// if there is a remaining separator, remove it
		if( cxa_stringUtils_startsWith_withLengths(currTopic, currTopicLen_bytes, "/", 1) )
		{
			currTopic++;
			currTopicLen_bytes--;
		}

		// we already know it's a request...but at this point, we want to make sure that we have the request prefix
		if( cxa_stringUtils_startsWith_withLengths(currTopic, currTopicLen_bytes, CXA_MQTT_RPCNODE_REQ_PREFIX, strlen(CXA_MQTT_RPCNODE_REQ_PREFIX)) )
		{
			// move the current topic forward (to discard the prefix)
			currTopic += strlen(CXA_MQTT_RPCNODE_REQ_PREFIX);
			currTopicLen_bytes -= strlen(CXA_MQTT_RPCNODE_REQ_PREFIX);

			// start looking for a method
			cxa_array_iterate(&superIn->methods, currMethodEntry, cxa_mqtt_rpc_node_methodEntry_t)
			{
				if( currMethodEntry == NULL ) continue;

				if( cxa_stringUtils_startsWith_withLengths(currTopic, currTopicLen_bytes, currMethodEntry->name, strlen(currMethodEntry->name)) )
				{
					cxa_logger_trace(&superIn->logger, "found method '%s'", currMethodEntry->name);

					// if we made it here we'll be sending a response
					cxa_linkedField_t *lf_payload, *lf_retPayload;
					cxa_mqtt_message_t* respMsg = prepForResponse(superIn, msgIn, &lf_payload, &lf_retPayload);
					if( respMsg == NULL ) return true;

					cxa_mqtt_rpc_methodRetVal_t retVal = CXA_MQTT_RPC_METHODRETVAL_SUCCESS;
					if( currMethodEntry->cb_method != NULL ) retVal = currMethodEntry->cb_method(superIn, lf_payload, lf_retPayload, currMethodEntry->userVar);
					sendResponse(superIn, retVal, respMsg);

					return true;
				}
			}

			// if we made it here, it is bound for a unknown method
			cxa_logger_log_untermString(&superIn->logger, CXA_LOG_LEVEL_WARN, "unknown method: '", currTopic, currTopicLen_bytes, "'");
			cxa_linkedField_t *lf_payload, *lf_retPayload;
			cxa_mqtt_message_t* respMsg = prepForResponse(superIn, msgIn, &lf_payload, &lf_retPayload);
			if( respMsg != NULL ) sendResponse(superIn, CXA_MQTT_RPC_METHODRETVAL_FAIL_METHOD_DNE, respMsg);

			return true;
		}

		// if we made it here...this must be destined for a subnode
		cxa_array_iterate(&superIn->subNodes, currSubNode, cxa_mqtt_rpc_node_t*)
		{
			if( currSubNode == NULL ) continue;
			if( ((*currSubNode)->scm_handleMessage_downstream != NULL) && (*currSubNode)->scm_handleMessage_downstream(*currSubNode, currTopic, currTopicLen_bytes, msgIn) ) return true;
		}

		// if we made it here, it is bound for an unknown subnode
		cxa_logger_log_untermString(&superIn->logger, CXA_LOG_LEVEL_WARN, "unknown subNode: '", currTopic, currTopicLen_bytes, "'");
		cxa_linkedField_t *lf_payload, *lf_retPayload;
		cxa_mqtt_message_t* respMsg = prepForResponse(superIn, msgIn, &lf_payload, &lf_retPayload);
		if( respMsg != NULL ) sendResponse(superIn, CXA_MQTT_RPC_METHODRETVAL_FAIL_NODE_DNE, respMsg);
		return true;
	}
	else if( cxa_mqtt_rpc_message_isActionableResponse(msgIn, &methodName, &methodNameLen_bytes, &id, &idLen_bytes) )
	{
		// this is a response...first, we should check to see if we were waiting for this...
		cxa_array_iterate(&superIn->outstandingRequests, currRequest, cxa_mqtt_rpc_node_outstandingRequest_t)
		{
			if( currRequest == NULL ) continue;

			if( cxa_stringUtils_equals_withLengths(currRequest->name, strlen(currRequest->name), methodName, methodNameLen_bytes) &&
				cxa_stringUtils_equals_withLengths(currRequest->id, strlen(currRequest->id), id, idLen_bytes) )
			{
				// we were expecting this response...get the return value (and remove leaving only parameters)
				cxa_linkedField_t* lf_payload;
				uint8_t retVal_raw;
				if( !cxa_mqtt_message_publish_getPayload(msgIn, &lf_payload) ||
					!cxa_linkedField_get_uint8(lf_payload, 0, retVal_raw) ||
					!cxa_linkedField_remove(lf_payload, 0, 1) )
				{
					cxa_logger_warn(&superIn->logger, "no return value found in response");
					return true;
				}

				if( currRequest->cb != NULL ) currRequest->cb(superIn, (cxa_mqtt_rpc_methodRetVal_t)retVal_raw, lf_payload, currRequest->userVar);

				// we're done with this request...remove it (so it doesn't timeout)
				cxa_array_remove(&superIn->outstandingRequests, currRequest);
				return true;
			}
		}

		// if we made it here, we need to pass to all subnodes so they can
		// individually decide if this was a message for which they were waiting
		cxa_array_iterate(&superIn->subNodes, currSubNode, cxa_mqtt_rpc_node_t*)
		{
			if( currSubNode == NULL ) continue;
			if( ((*currSubNode)->scm_handleMessage_downstream != NULL) && (*currSubNode)->scm_handleMessage_downstream(*currSubNode, remainingTopicIn, remainingTopicLen_bytesIn, msgIn) ) return true;
		}
	}

	return false;
}


static cxa_mqtt_message_t* prepForResponse(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_message_t* reqMsgIn, cxa_linkedField_t **lf_payloadIn, cxa_linkedField_t **lf_retPayloadIn)
{
	cxa_assert(nodeIn);
	cxa_assert(reqMsgIn);
	cxa_assert(lf_payloadIn);
	cxa_assert(lf_retPayloadIn);

	char *reqTopicName, *respTopicName;
	uint16_t reqTopicNameLen_bytes, respTopicNameLen_bytes;

	cxa_mqtt_message_t* respMsg = cxa_mqtt_messageFactory_getFreeMessage_empty();
	if( (respMsg == NULL) || !cxa_mqtt_message_publish_init(respMsg, false, CXA_MQTT_QOS_ATMOST_ONCE, false, "", 0, NULL, 0) ||				// get the response and init it
		!cxa_mqtt_message_publish_getTopicName(reqMsgIn, &reqTopicName, &reqTopicNameLen_bytes) ||											// setup the response topic
		!cxa_mqtt_message_publish_topicName_prependString_withLength(respMsg, reqTopicName, reqTopicNameLen_bytes) ||
		!cxa_mqtt_message_publish_getTopicName(respMsg, &respTopicName, &respTopicNameLen_bytes) ||
		!cxa_stringUtils_replaceFirstOccurance_withLengths(respTopicName, respTopicNameLen_bytes,
														   CXA_MQTT_RPCNODE_REQ_PREFIX, strlen(CXA_MQTT_RPCNODE_REQ_PREFIX),
														   CXA_MQTT_RPCNODE_RESP_PREFIX, strlen(CXA_MQTT_RPCNODE_RESP_PREFIX)) ||
		!cxa_mqtt_message_publish_getPayload(reqMsgIn, lf_payloadIn) || !cxa_mqtt_message_publish_getPayload(respMsg, lf_retPayloadIn) )	// setup payloads
	{
		cxa_logger_warn(&nodeIn->logger, "error reserving/init'ing response");
		return NULL;
	}

	return respMsg;
}


static void sendResponse(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_methodRetVal_t retValIn, cxa_mqtt_message_t *responseMessageIn)
{
	cxa_assert(nodeIn);
	cxa_assert(responseMessageIn);

	cxa_linkedField_t* lf_retPayload;
	if( cxa_mqtt_message_publish_getPayload(responseMessageIn, &lf_retPayload) &&
		cxa_linkedField_insert_uint8(lf_retPayload, 0, (uint8_t)retValIn) )
	{
		nodeIn->scm_handleMessage_upstream(nodeIn, responseMessageIn);
	}else cxa_logger_warn(&nodeIn->logger, "error sending response");
	cxa_mqtt_messageFactory_decrementMessageRefCount(responseMessageIn);
}

