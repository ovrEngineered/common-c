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
#include "cxa_mqtt_rpc_node_bridge_single.h"


// ******** includes ********
#include <string.h>
#include <cxa_assert.h>
#include <cxa_mqtt_messageFactory.h>
#include <cxa_mqtt_message_connect.h>
#include <cxa_mqtt_message_connack.h>
#include <cxa_mqtt_message_pingResponse.h>
#include <cxa_mqtt_message_publish.h>
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

static bool rpcCb_catchall(cxa_mqtt_rpc_node_t *const superIn, char *const remainingTopicIn, size_t remainingTopicLen_bytes, cxa_mqtt_message_t *const msgIn, void* userVarIn);

static void protoParseCb_onPacketReceived(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn);
static void handleMessage_publish(cxa_mqtt_rpc_node_bridge_single_t *const nodeIn, cxa_mqtt_message_t *const msgIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_mqtt_rpc_node_bridge_single_init(cxa_mqtt_rpc_node_bridge_single_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn,
										 cxa_protocolParser_mqtt_t *const mppIn, const char *nameFmtIn, ...)
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
	cxa_mqtt_rpc_node_setCatchAll(&nodeIn->super.super, rpcCb_catchall, (void*)nodeIn);

	// set some defaults
	nodeIn->clientId[0] = 0;
	nodeIn->hasClientAuthed = false;
	nodeIn->cb_localAuth = NULL;
	nodeIn->localAuthUserVar = NULL;

	// listen specifically for publishes
	cxa_protocolParser_addPacketListener(&nodeIn->super.mpp->super, protoParseCb_onPacketReceived, (void*)nodeIn);
}


void cxa_mqtt_rpc_node_bridge_single_setAuthCb(cxa_mqtt_rpc_node_bridge_single_t *const nodeIn, cxa_mqtt_rpc_node_bridge_cb_authenticateClient_t authCbIn, void *const userVarIn)
{
	cxa_assert(nodeIn);

	nodeIn->cb_localAuth = authCbIn;
	nodeIn->localAuthUserVar = userVarIn;
}


// ******** local function implementations ********
static cxa_mqtt_rpc_node_bridge_authorization_t bridgeAuthCb(char *const clientIdIn, size_t clientIdLen_bytes,
															 char *const usernameIn, size_t usernameLen_bytesIn,
															 uint8_t *const passwordIn, size_t passwordLen_bytesIn,
															 void *userVarIn)
{
	cxa_mqtt_rpc_node_bridge_single_t* nodeIn = (cxa_mqtt_rpc_node_bridge_single_t*)userVarIn;
	cxa_assert(nodeIn);

	// we need an authorization callback first...
	if( nodeIn->cb_localAuth == NULL ) return CXA_MQTT_RPC_NODE_BRIDGE_AUTH_IGNORE;

	// make sure the client ID is an appropriate length
	if( clientIdLen_bytes > (sizeof(nodeIn->clientId)-1) )
	{
		cxa_logger_warn(&nodeIn->super.super.logger, "remote clientId too long");
		return CXA_MQTT_RPC_NODE_BRIDGE_AUTH_IGNORE;
	}

	// call our local function
	cxa_mqtt_rpc_node_bridge_authorization_t retVal = nodeIn->cb_localAuth(clientIdIn, clientIdLen_bytes,
																		   usernameIn, usernameLen_bytesIn,
																		   passwordIn, passwordLen_bytesIn,
																		   nodeIn->localAuthUserVar);
	switch( retVal )
	{
		case CXA_MQTT_RPC_NODE_BRIDGE_AUTH_ALLOW:
			cxa_logger_info(&nodeIn->super.super.logger, "remote client connected");

			// store a local copy of the clientID (null terminated)
			memcpy(nodeIn->clientId, clientIdIn, clientIdLen_bytes);
			nodeIn->clientId[clientIdLen_bytes] = 0;

			nodeIn->hasClientAuthed = true;
			break;

		case CXA_MQTT_RPC_NODE_BRIDGE_AUTH_DISALLOW:
			nodeIn->hasClientAuthed = false;
			break;

		default:
			break;
	}

	return retVal;
}


static bool rpcCb_catchall(cxa_mqtt_rpc_node_t *const superIn, char *const remainingTopicIn, size_t remainingTopicLen_bytes, cxa_mqtt_message_t *const msgIn, void* userVarIn)
{
	cxa_mqtt_rpc_node_bridge_single_t* nodeIn = (cxa_mqtt_rpc_node_bridge_single_t*)superIn;
	cxa_assert(nodeIn);

	cxa_logger_log_untermString(&nodeIn->super.super.logger, CXA_LOG_LEVEL_TRACE, "catchall: '", remainingTopicIn, remainingTopicLen_bytes, "'");

	// make sure we have an authenticated client first
	if( !nodeIn->hasClientAuthed )
	{
		cxa_logger_warn(&nodeIn->super.super.logger, "client not yet auth'd, dropping");
		return false;
	}

	// go ahead and forward
	cxa_logger_trace(&nodeIn->super.super.logger, "forwarding to single: '%s'", nodeIn->clientId);

	if( !cxa_mqtt_message_publish_topicName_trimToPointer(msgIn, remainingTopicIn) ||
		!cxa_mqtt_message_publish_topicName_prependCString(msgIn, "/") ||
		!cxa_mqtt_message_publish_topicName_prependCString(msgIn, nodeIn->clientId) )
	{
		cxa_logger_warn(&nodeIn->super.super.logger, "error remapping topic name, dropping");
		return true;		// return true because we _should_ have handled this
	}

	// send the message
	if( !cxa_protocolParser_writePacket(&nodeIn->super.mpp->super, cxa_mqtt_message_getBuffer(msgIn)) )
	{
		cxa_logger_warn(&nodeIn->super.super.logger, "error forwarding message");
	}
	return true;
}


static void protoParseCb_onPacketReceived(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn)
{
	cxa_mqtt_rpc_node_bridge_single_t* nodeIn = (cxa_mqtt_rpc_node_bridge_single_t*)userVarIn;
	cxa_assert(nodeIn);

	cxa_mqtt_message_t* msg = cxa_mqtt_messageFactory_getMessage_byBuffer(packetIn);
	if( msg == NULL ) return;

	cxa_mqtt_message_type_t msgType = cxa_mqtt_message_getType(msg);
	switch( msgType )
	{
		case CXA_MQTT_MSGTYPE_PUBLISH:
			handleMessage_publish(nodeIn, msg);
			break;

		default:
			break;
	}
}


static void handleMessage_publish(cxa_mqtt_rpc_node_bridge_single_t *const nodeIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(nodeIn);
	cxa_assert(msgIn);

	// get our topic name and length
	char* topicName;
	uint16_t topicLen_bytes;
	if( !cxa_mqtt_message_publish_getTopicName(msgIn, &topicName, &topicLen_bytes) ) return;
	cxa_logger_log_untermString(&nodeIn->super.super.logger, CXA_LOG_LEVEL_TRACE, "got PUBLISH '", topicName, topicLen_bytes, "'");

	// ensure that our publish topic actually has our clientId in it
	if( !cxa_stringUtils_contains_withLengths(topicName, topicLen_bytes, nodeIn->clientId, strlen(nodeIn->clientId)) ) return;

	// remember whether this is a request
	bool isResponse = cxa_stringUtils_startsWith(topicName, CXA_MQTT_RPCNODE_RESP_PREFIX);

	// we'll need to do some unmapping to get the topic name inline with our node structure
	char* startOfClientId;
	if( (startOfClientId = strstr(topicName, nodeIn->clientId)) == NULL ) return;

	// ok...get rid of everything up to, and including, the clientId (+1 is for separator)
	if( !cxa_mqtt_message_publish_topicName_trimToPointer(msgIn, startOfClientId+strlen(nodeIn->clientId)+1) ) return;

	// now we need to prepend our node structure
	cxa_mqtt_rpc_node_t* currNode = &nodeIn->super.super;
	while( currNode != NULL )
	{
		if( !cxa_mqtt_message_publish_topicName_prependCString(msgIn, "/") ||
			!cxa_mqtt_message_publish_topicName_prependCString(msgIn, currNode->name) ) return;

		// if we're the root node, we need to prepend our root prefix (if it exists)
		if( currNode->isRootNode )
		{
			if( !cxa_mqtt_message_publish_topicName_prependCString(msgIn, cxa_mqtt_rpc_node_root_getPrefix((cxa_mqtt_rpc_node_root_t*)currNode)) ) return;
		}
		currNode = currNode->parentNode;
	}

	// if we made it here, we should have a proper topic name (potentially without the response prefix)
	if( isResponse && !cxa_mqtt_message_publish_topicName_prependCString(msgIn, CXA_MQTT_RPCNODE_RESP_PREFIX) ) return;

	// now that everything is unmapped...toss to the root node for handling
	cxa_mqtt_rpc_node_root_t* rootNode = cxa_mqtt_rpc_node_getRootNode(&nodeIn->super.super);
	if( rootNode != NULL ) cxa_mqtt_rpc_node_root_handleInternalPublish(rootNode, msgIn);
}
