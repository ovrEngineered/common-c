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
static bool rpcCb_catchall(cxa_mqtt_rpc_node_t *const nodeIn, char *const remainingTopicIn, size_t remainingTopicLen_bytes, cxa_mqtt_message_t *const msgIn, void* userVarIn);


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
	cxa_mqtt_rpc_node_setCatchAll(&nodeIn->super.super, rpcCb_catchall, (void*)nodeIn);

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

	cxa_mqtt_rpc_node_bridge_multi_remoteNodeEntry_t newEntry;



	// we need an authorization callback first...
	return CXA_MQTT_RPC_NODE_BRIDGE_AUTH_IGNORE;
}


static bool rpcCb_catchall(cxa_mqtt_rpc_node_t *const superIn, char *const remainingTopicIn, size_t remainingTopicLen_bytes, cxa_mqtt_message_t *const msgIn, void* userVarIn)
{
	cxa_mqtt_rpc_node_bridge_multi_t* nodeIn = (cxa_mqtt_rpc_node_bridge_multi_t*)superIn;
	cxa_assert(nodeIn);

	cxa_logger_log_untermString(&nodeIn->super.super.logger, CXA_LOG_LEVEL_TRACE, "catchall: '", remainingTopicIn, remainingTopicLen_bytes, "'");



	// iterate through our mapped nodes to see if 'foo' matches a node we know...
	cxa_array_iterate(&nodeIn->remoteNodes, currRemNode, cxa_mqtt_rpc_node_bridge_multi_remoteNodeEntry_t)
	{
		if( currRemNode == NULL ) continue;

		size_t currMappedNameLen_bytes = strlen(currRemNode->mappedName);
		if( (currMappedNameLen_bytes <= remainingTopicLen_bytes) && cxa_stringUtils_startsWith(remainingTopicIn, currRemNode->mappedName) )
		{
			// we have a match!
			cxa_logger_trace(&nodeIn->super.super.logger, "found match: '%s'", currRemNode->mappedName);

			// advance to the end of our mapped name (should be a path separator)
			char* newTopicName = remainingTopicIn + currMappedNameLen_bytes;
			size_t newTopicNameLen_bytes = remainingTopicLen_bytes - currMappedNameLen_bytes;

			// now, we need to massage the topic of this message too look something like:
			// ~/<foo's clientId>/::bar
			if( (newTopicNameLen_bytes < 1) || (*newTopicName != '/') ||
					!cxa_mqtt_message_publish_topicName_trimToPointer(msgIn, newTopicName) ||
					!cxa_mqtt_message_publish_topicName_prependCString(msgIn, currRemNode->clientId) ||
					!cxa_mqtt_message_publish_topicName_prependCString(msgIn, "~/") )
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
	}

	// if we made it here, we found no matching remote nodes
	cxa_logger_log_untermString(&nodeIn->super.super.logger, CXA_LOG_LEVEL_WARN, "couldn't handle '", remainingTopicIn, remainingTopicLen_bytes, "'");
	return false;
}
