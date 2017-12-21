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
#include "cxa_mqtt_rpc_node_bridge_multi.h"


// ******** includes ********
#include <string.h>
#include <cxa_assert.h>
#include <cxa_mqtt_messageFactory.h>
#include <cxa_mqtt_message_connect.h>
#include <cxa_mqtt_message_connack.h>
#include <cxa_mqtt_message_pingResponse.h>
#include <cxa_mqtt_message_publish.h>
#include <cxa_mqtt_rpc_message.h>
#include <cxa_mqtt_rpc_node_root.h>
#include <cxa_stringUtils.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_mqtt_rpc_node_bridge_authorization_t bridgeAuthCb(char *const clientIdIn, size_t clientIdLen_bytes,
															 char *const usernameIn, size_t usernameLen_bytesIn,
															 uint8_t *const passwordIn, size_t passwordLen_bytesIn,
															 void *userVarIn);

static void scm_handleMessage_upstream(cxa_mqtt_rpc_node_t *const superIn, cxa_mqtt_message_t *const msgIn);
static bool scm_handleMessage_downstream(cxa_mqtt_rpc_node_t *const superIn,
										 char *const remainingTopicIn, uint16_t remainingTopicLen_bytesIn,
										 cxa_mqtt_message_t *const msgIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_mqtt_rpc_node_bridge_multi_init(cxa_mqtt_rpc_node_bridge_multi_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn,
										 cxa_protocolParser_mqtt_t *const mppIn,
										 const char *nameFmtIn, ...)
{
	cxa_assert(nodeIn);
	cxa_assert(parentNodeIn);
	cxa_assert(mppIn);
	cxa_assert(nameFmtIn);

	// initialize our super class
	va_list varArgs;
	va_start(varArgs, nameFmtIn);
	cxa_mqtt_rpc_node_bridge_vinit(&nodeIn->super, parentNodeIn, mppIn, bridgeAuthCb, (void*)nodeIn, nameFmtIn, varArgs);
	va_end(varArgs);

	// setup our subclass methods / overrides
	nodeIn->super.super.scm_handleMessage_upstream = scm_handleMessage_upstream;
	nodeIn->super.super.scm_handleMessage_downstream = scm_handleMessage_downstream;

	// set some defaults
	nodeIn->cb_localAuth = NULL;
	nodeIn->localAuthUserVar = NULL;

	// setup our remote nodes
	cxa_array_initStd(&nodeIn->remoteNodes, nodeIn->remoteNodes_raw);
}


void cxa_mqtt_rpc_node_bridge_multi_setAuthCb(cxa_mqtt_rpc_node_bridge_multi_t *const nodeIn, cxa_mqtt_rpc_node_bridge_multi_cb_authenticateClient_t authCbIn, void *const userVarIn)
{
	cxa_assert(nodeIn);

	nodeIn->cb_localAuth = authCbIn;
	nodeIn->localAuthUserVar = userVarIn;
}


size_t cxa_mqtt_rpc_node_bridge_multi_getNumRemoteNodes(cxa_mqtt_rpc_node_bridge_multi_t *const nodeIn)
{
	cxa_assert(nodeIn);
	return cxa_array_getSize_elems(&nodeIn->remoteNodes);
}


void cxa_mqtt_rpc_node_bridge_multi_clearRemoteNodes(cxa_mqtt_rpc_node_bridge_multi_t *const nodeIn)
{
	cxa_assert(nodeIn);

	cxa_array_clear(&nodeIn->remoteNodes);
}


// ******** local function implementations ********
static cxa_mqtt_rpc_node_bridge_authorization_t bridgeAuthCb(char *const clientIdIn, size_t clientIdLen_bytes,
															 char *const usernameIn, size_t usernameLen_bytesIn,
															 uint8_t *const passwordIn, size_t passwordLen_bytesIn,
															 void *userVarIn)
{
	cxa_mqtt_rpc_node_bridge_multi_t* nodeIn = (cxa_mqtt_rpc_node_bridge_multi_t*)userVarIn;
	cxa_assert(nodeIn);

	// we need an authorization callback first...
	if( nodeIn->cb_localAuth == NULL ) return CXA_MQTT_RPC_NODE_BRIDGE_AUTH_IGNORE;

	// check out our current remote nodes to see if someone is connecting again (reboot maybe?)
	cxa_array_iterate(&nodeIn->remoteNodes, currRemoteNode, cxa_mqtt_rpc_node_bridge_multi_remoteNodeEntry_t)
	{
		if( currRemoteNode ) continue;

		if( cxa_stringUtils_equals(currRemoteNode->clientId, clientIdIn) )
		{
			cxa_logger_log_untermString(&nodeIn->super.super.logger, CXA_LOG_LEVEL_DEBUG, "reauth attempt for '", clientIdIn, clientIdLen_bytes, "'");
			return CXA_MQTT_RPC_NODE_BRIDGE_AUTH_ALLOW;
		}
	}

	// make sure we have space
	if( cxa_array_isFull(&nodeIn->remoteNodes) )
	{
		cxa_logger_warn(&nodeIn->super.super.logger, "too many remote dropping");
		return CXA_MQTT_RPC_NODE_BRIDGE_AUTH_IGNORE;
	}

	// get our new entry ready to record the remote client
	cxa_mqtt_rpc_node_bridge_multi_remoteNodeEntry_t newEntry;
	newEntry.clientId[0] = 0;
	newEntry.mappedName[0] = 0;

	// make sure the client ID OK
	if( clientIdLen_bytes >= sizeof(newEntry.clientId) )
	{
		cxa_logger_warn(&nodeIn->super.super.logger, "remote clientId too long");
		return CXA_MQTT_RPC_NODE_BRIDGE_AUTH_IGNORE;
	}
	cxa_stringUtils_concat_withLengths(newEntry.clientId, sizeof(newEntry.clientId), clientIdIn, clientIdLen_bytes);

	// call _our_ authorization callback
	cxa_mqtt_rpc_node_bridge_authorization_t retVal = nodeIn->cb_localAuth(clientIdIn, clientIdLen_bytes,
																		   usernameIn, usernameLen_bytesIn,
																		   passwordIn, passwordLen_bytesIn,
																		   newEntry.mappedName, sizeof(newEntry.mappedName),
																		   nodeIn->localAuthUserVar);
	if( retVal != CXA_MQTT_RPC_NODE_BRIDGE_AUTH_ALLOW ) return retVal;

	// if we made it here, we are allowing it (assuming we have space)
	cxa_assert( cxa_array_append(&nodeIn->remoteNodes, &newEntry) );

	return CXA_MQTT_RPC_NODE_BRIDGE_AUTH_ALLOW;
}


static void scm_handleMessage_upstream(cxa_mqtt_rpc_node_t *const superIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_mqtt_rpc_node_bridge_multi_t* nodeIn = (cxa_mqtt_rpc_node_bridge_multi_t*)superIn;
	cxa_assert(nodeIn);
	cxa_assert(msgIn);

	// get our topic name and length
	char* topicName;
	uint16_t topicNameLen_bytes;
	if( !cxa_mqtt_message_publish_getTopicName(msgIn, &topicName, &topicNameLen_bytes) ||
		(topicName == NULL) || (topicNameLen_bytes == 0) ) return;

	// this _may_ be our client state message...if so, we don't need to do anything
	if( cxa_stringUtils_endsWith_withLengths(topicName, topicNameLen_bytes, CXA_MQTT_RPCNODE_CONNSTATE_STREAM_NAME) ) return;

	cxa_logger_log_untermString(&nodeIn->super.super.logger, CXA_LOG_LEVEL_TRACE, "<< '", topicName, topicNameLen_bytes, "'");

	if( cxa_stringUtils_startsWith_withLengths(topicName, topicNameLen_bytes, "/", 1) ||
		cxa_stringUtils_startsWith_withLengths(topicName, topicNameLen_bytes, CXA_MQTT_RPCNODE_LOCALROOT_PREFIX, strlen(CXA_MQTT_RPCNODE_LOCALROOT_PREFIX)))
	{
		// this message is addressed from the global root or the local root respectively...send it up!
		if( superIn->parentNode != NULL ) superIn->parentNode->scm_handleMessage_upstream(superIn->parentNode, msgIn);

		// now that we're done handling the message...clear its topic (so none else can handle it)
		cxa_mqtt_message_publish_topicName_clear(msgIn);
	}
	else
	{
		// we'll need to do some remapping here...

		// ensure that our publish topic actually matches one of our remote nodes
		cxa_mqtt_rpc_node_bridge_multi_remoteNodeEntry_t* targetRne = NULL;
		cxa_array_iterate(&nodeIn->remoteNodes, currRemNode, cxa_mqtt_rpc_node_bridge_multi_remoteNodeEntry_t)
		{
			if( currRemNode == NULL ) continue;

			if( cxa_stringUtils_contains_withLengths(topicName, topicNameLen_bytes, currRemNode->clientId, strlen(currRemNode->clientId)) )
			{
				targetRne = currRemNode;
				break;
			}
		}
		if( targetRne == NULL ) return;

		// ok...get rid of everything up to, and including, the clientId (+1 is for separator)
		if( !cxa_mqtt_message_publish_topicName_trimToPointer(msgIn, topicName+strlen(targetRne->clientId)+1) ) return;

		// prepend our mapped name first
		if( !cxa_mqtt_message_publish_topicName_prependCString(msgIn, "/") ||
			!cxa_mqtt_message_publish_topicName_prependCString(msgIn, targetRne->mappedName) ) return;

		// now we need to prepend our node structure
		cxa_mqtt_rpc_node_t* currNode = &nodeIn->super.super;
		while( (currNode != NULL) && (currNode->parentNode != NULL) )
		{
			if( !cxa_mqtt_message_publish_topicName_prependCString(msgIn, "/") ||
				!cxa_mqtt_message_publish_topicName_prependCString(msgIn, currNode->name) ) return;

			currNode = currNode->parentNode;
		}
		// and finally our local root prefix
		if( !cxa_mqtt_message_publish_topicName_prependCString(msgIn, CXA_MQTT_RPCNODE_LOCALROOT_PREFIX) ) return;

		// message should be now be mapped properly...hand upstream!
		if( superIn->parentNode != NULL ) superIn->parentNode->scm_handleMessage_upstream(superIn->parentNode, msgIn);

		// now that we're done handling the message...clear its topic (so none else can handle it)
		cxa_mqtt_message_publish_topicName_clear(msgIn);
	}
}


static bool scm_handleMessage_downstream(cxa_mqtt_rpc_node_t *const superIn,
										 char *const remainingTopicIn, uint16_t remainingTopicLen_bytesIn,
										 cxa_mqtt_message_t *const msgIn)
{
	cxa_mqtt_rpc_node_bridge_multi_t* nodeIn = (cxa_mqtt_rpc_node_bridge_multi_t*)superIn;
	cxa_assert(nodeIn);
	cxa_assert(remainingTopicIn);
	cxa_assert(msgIn);

	cxa_logger_log_untermString(&superIn->logger, CXA_LOG_LEVEL_TRACE, ">> '", remainingTopicIn, remainingTopicLen_bytesIn, "'");

	char *methodName, *id;
	size_t methodNameLen_bytes, idLen_bytes;

//	// depends what type of message it is...
//	if( cxa_mqtt_rpc_message_isActionableRequest(msgIn, NULL, NULL, NULL, NULL) )
//	{
//		// this is a request
//
//		// iterate through our mapped nodes to see if we match a node we know...
//		cxa_array_iterate(&nodeIn->remoteNodes, currRemNode, cxa_mqtt_rpc_node_bridge_multi_remoteNodeEntry_t)
//		{
//			if( currRemNode == NULL ) continue;
//
//			size_t currMappedNameLen_bytes = strlen(currRemNode->mappedName);
//			if( (currMappedNameLen_bytes <= remainingTopicLen_bytesIn) && cxa_stringUtils_startsWith(remainingTopicIn, currRemNode->mappedName) )
//			{
//				// we have a match!
//				cxa_logger_trace(&nodeIn->super.super.logger, "found match: '%s'", currRemNode->mappedName);
//
//				// advance to the end of our mapped name (should be a path separator)
//				char* newTopicName = remainingTopicIn + currMappedNameLen_bytes;
//				size_t newTopicNameLen_bytes = remainingTopicLen_bytesIn - currMappedNameLen_bytes;
//
//				// now, we need to massage the topic of this message too look something like:
//				// <foo's clientId>/::bar
//
//				if( (newTopicNameLen_bytes < 1) || (*newTopicName != '/') ||
//						!cxa_mqtt_message_publish_topicName_trimToPointer(msgIn, newTopicName) ||
//						!cxa_mqtt_message_publish_topicName_prependCString(msgIn, currRemNode->clientId) )
//				{
//					cxa_logger_warn(&nodeIn->super.super.logger, "error remapping topic name, dropping");
//					return true;		// return true because we _should_ have handled this
//				}
//
//				// send the message
//				if( !cxa_protocolParser_writePacket(&nodeIn->super.mpp->super, cxa_mqtt_message_getBuffer(msgIn)) )
//				{
//					cxa_logger_warn(&nodeIn->super.super.logger, "error forwarding message");
//				}
//
//				return true;
//			}
//		}
//	}
//	else if( cxa_mqtt_rpc_message_isActionableResponse(msgIn, &methodName, &methodNameLen_bytes, &id, &idLen_bytes) )
//	{
//		// this is a response...first, we should check to see if we were waiting for this...
//		cxa_array_iterate(&superIn->outstandingRequests, currRequest, cxa_mqtt_rpc_node_outstandingRequest_t)
//		{
//			if( currRequest == NULL ) continue;
//
//			if( cxa_stringUtils_equals_withLengths(currRequest->name, strlen(currRequest->name), methodName, methodNameLen_bytes) &&
//				cxa_stringUtils_equals_withLengths(currRequest->id, strlen(currRequest->id), id, idLen_bytes) )
//			{
//				// we were expecting this response...get the return value (and remove leaving only parameters)
//				cxa_linkedField_t* lf_payload;
//				uint8_t retVal_raw;
//				if( !cxa_mqtt_message_publish_getPayload(msgIn, &lf_payload) ||
//					!cxa_linkedField_get_uint8(lf_payload, 0, retVal_raw) ||
//					!cxa_linkedField_remove(lf_payload, 0, 1) )
//				{
//					cxa_logger_warn(&superIn->logger, "no return value found in response");
//					return true;
//				}
//
//				if( currRequest->cb != NULL ) currRequest->cb(superIn, (cxa_mqtt_rpc_methodRetVal_t)retVal_raw, lf_payload, currRequest->userVar);
//
//				// we're done with this request...remove it (so it doesn't timeout)
//				cxa_array_remove(&superIn->outstandingRequests, currRequest);
//				return true;
//			}
//		}
//	}

	return false;
}
