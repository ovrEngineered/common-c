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

	// setup our subclass methods / overrides
	nodeIn->super.super.scm_handleMessage_upstream = scm_handleMessage_upstream;
	nodeIn->super.super.scm_handleMessage_downstream = scm_handleMessage_downstream;

	// set some defaults
	nodeIn->clientId[0] = 0;
	nodeIn->hasClientAuthed = false;
	nodeIn->cb_localAuth = NULL;
	nodeIn->localAuthUserVar = NULL;
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


static void scm_handleMessage_upstream(cxa_mqtt_rpc_node_t *const superIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_mqtt_rpc_node_bridge_single_t* nodeIn = (cxa_mqtt_rpc_node_bridge_single_t*)superIn;
	cxa_assert(nodeIn);
	cxa_assert(msgIn);

	// make sure we actually have a client
	if( !nodeIn->hasClientAuthed ) return;

	// get our topic name and length
	char* topicName;
	uint16_t topicNameLen_bytes;
	if( !cxa_mqtt_message_publish_getTopicName(msgIn, &topicName, &topicNameLen_bytes) ) return;

	// this _may_ be our client state message...if so, we don't need to do anything
	if( cxa_stringUtils_endsWith_withLengths(topicName, topicNameLen_bytes, CXA_MQTT_RPCNODE_CONNSTATE_TOPIC) ) return;

	cxa_logger_log_untermString(&nodeIn->super.super.logger, CXA_LOG_LEVEL_TRACE, "<< '", topicName, topicNameLen_bytes, "'");

	if( cxa_stringUtils_startsWith_withLengths(topicName, topicNameLen_bytes, "/", 1) ||
			cxa_stringUtils_startsWith_withLengths(topicName, topicNameLen_bytes, CXA_MQTT_RPCNODE_LOCALROOT_PREFIX, strlen(CXA_MQTT_RPCNODE_LOCALROOT_PREFIX)))
	{
		// this message is addressed from the global root or the local root respectively...send it up!
		if( superIn->parentNode != NULL ) superIn->parentNode->scm_handleMessage_upstream(superIn->parentNode, msgIn);
	}
	else
	{
		// we'll need to do some remapping here...

		// ensure that our publish topic actually has our clientId in it
		if( !cxa_stringUtils_startsWith_withLengths(topicName, topicNameLen_bytes, nodeIn->clientId, strlen(nodeIn->clientId)) ) return;

		// ok...get rid of everything up to, and including, the clientId (+1 is for separator)
		if( !cxa_mqtt_message_publish_topicName_trimToPointer(msgIn, topicName+strlen(nodeIn->clientId)+1) ) return;

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
	}
}


static bool scm_handleMessage_downstream(cxa_mqtt_rpc_node_t *const superIn,
										 char *const remainingTopicIn, uint16_t remainingTopicLen_bytesIn,
										 cxa_mqtt_message_t *const msgIn)
{
	cxa_mqtt_rpc_node_bridge_single_t* nodeIn = (cxa_mqtt_rpc_node_bridge_single_t*)superIn;
	cxa_assert(nodeIn);
	cxa_assert(remainingTopicIn);
	cxa_assert(msgIn);

	cxa_logger_log_untermString(&superIn->logger, CXA_LOG_LEVEL_TRACE, ">> '", remainingTopicIn, remainingTopicLen_bytesIn, "'");

	// depends what type of message it is...
	if( cxa_mqtt_rpc_message_isActionableRequest(msgIn, NULL, NULL, NULL, NULL) )
	{
		// this is a request

		// make sure that the topic starts with our name
		size_t nodeNameLen_bytes = strlen(superIn->name);
		if( !cxa_stringUtils_startsWith_withLengths(remainingTopicIn, remainingTopicLen_bytesIn, superIn->name, nodeNameLen_bytes) ) return false;

		// so far so good...remove ourselves from the topic
		char* currTopic = remainingTopicIn + nodeNameLen_bytes;
		size_t currTopicLen_bytes = remainingTopicLen_bytesIn - nodeNameLen_bytes;

		// if there is a remaining separator, remove it
		if( cxa_stringUtils_startsWith_withLengths(currTopic, currTopicLen_bytes, "/", 1) )
		{
			currTopic++;
			currTopicLen_bytes--;
		}

		// now make sure we have an authenticated client
		if( !nodeIn->hasClientAuthed )
		{
			cxa_logger_warn(&nodeIn->super.super.logger, "client not yet auth'd, dropping");
			return false;
		}

		// go ahead and forward
		cxa_logger_trace(&nodeIn->super.super.logger, "forwarding to single: '%s'", nodeIn->clientId);

		if( !cxa_mqtt_message_publish_topicName_trimToPointer(msgIn, currTopic) ||
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

	return false;
}
