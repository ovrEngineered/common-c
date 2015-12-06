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
#include "cxa_mqtt_rpc_node_bridge.h"


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
static void protoParseCb_onPacketReceived(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn);

static void handleMessage_connect(cxa_mqtt_rpc_node_bridge_t *const nodeIn, cxa_mqtt_message_t *const msgIn);
static void handleMessage_pingReq(cxa_mqtt_rpc_node_bridge_t *const nodeIn, cxa_mqtt_message_t *const msgIn);
static void handleMessage_subscribe(cxa_mqtt_rpc_node_bridge_t *const nodeIn, cxa_mqtt_message_t *const msgIn);
static void handleMessage_publish(cxa_mqtt_rpc_node_bridge_t *const nodeIn, cxa_mqtt_message_t *const msgIn);

static void sendMessage_connack(cxa_mqtt_rpc_node_bridge_t *const nodeIn, bool isSessionPresentIn, cxa_mqtt_connAck_returnCode_t retCodeIn);

static bool rpcCb_catchall(cxa_mqtt_rpc_node_t *const superIn, char *const remainingTopicIn, size_t remainingTopicLen_bytes, cxa_mqtt_message_t *const msgIn, void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_mqtt_rpc_node_bridge_init(cxa_mqtt_rpc_node_bridge_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn, char *const nameIn,
								   cxa_ioStream_t *const iosIn, cxa_timeBase_t *const timeBaseIn)
{
	cxa_assert(nodeIn);
	cxa_assert(iosIn);
	cxa_assert(timeBaseIn);

	// initialize our super class
	cxa_mqtt_rpc_node_init(&nodeIn->super, parentNodeIn, nameIn);
	cxa_mqtt_rpc_node_setCatchAll(&nodeIn->super, rpcCb_catchall, NULL);

	// set some defaults
	nodeIn->cb_auth = NULL;
	nodeIn->userVar_auth = NULL;

	// get a message (and buffer) for our protocol parser
	cxa_mqtt_message_t* msg = cxa_mqtt_messageFactory_getFreeMessage_empty();
	cxa_assert(msg);

	// setup our protocol parser
	cxa_protocolParser_mqtt_init(&nodeIn->mpp, iosIn, msg->buffer, timeBaseIn);
	cxa_protocolParser_addPacketListener(&nodeIn->mpp.super, protoParseCb_onPacketReceived, (void*)nodeIn);

	// setup our remote nodes
	cxa_array_initStd(&nodeIn->remoteNodes, nodeIn->remoteNodes_raw);
}


void cxa_mqtt_rpc_node_bridge_setAuthenticationCb(cxa_mqtt_rpc_node_bridge_t *const nodeIn, cxa_mqtt_rpc_node_bridge_cb_authenticateClient_t cb_authIn, void* userVarIn)
{
	cxa_assert(nodeIn);

	nodeIn->cb_auth = cb_authIn;
	nodeIn->userVar_auth = userVarIn;
}


void cxa_mqtt_rpc_node_bridge_update(cxa_mqtt_rpc_node_bridge_t *const nodeIn)
{
	cxa_assert(nodeIn);

	cxa_protocolParser_mqtt_update(&nodeIn->mpp);
}


// ******** local function implementations ********
static void protoParseCb_onPacketReceived(cxa_fixedByteBuffer_t *const packetIn, void *const userVarIn)
{
	cxa_mqtt_rpc_node_bridge_t* nodeIn = (cxa_mqtt_rpc_node_bridge_t*)userVarIn;
	cxa_assert(nodeIn);

	cxa_mqtt_message_t* msg = cxa_mqtt_messageFactory_getMessage_byBuffer(packetIn);
	if( msg == NULL ) return;

	cxa_mqtt_message_type_t msgType = cxa_mqtt_message_getType(msg);
	switch( msgType )
	{
		case CXA_MQTT_MSGTYPE_CONNECT:
			handleMessage_connect(nodeIn, msg);
			break;

		case CXA_MQTT_MSGTYPE_PINGREQ:
			handleMessage_pingReq(nodeIn, msg);
			break;

		case CXA_MQTT_MSGTYPE_SUBSCRIBE:
			handleMessage_subscribe(nodeIn, msg);
			break;

		case CXA_MQTT_MSGTYPE_PUBLISH:
			handleMessage_publish(nodeIn, msg);
			break;

		default:
			cxa_logger_trace(&nodeIn->super.logger, "got unknown msgType: %d", msgType);
			break;
	}
}


static void handleMessage_connect(cxa_mqtt_rpc_node_bridge_t *const nodeIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(nodeIn);

	// can't do anything unless we have an auth callback setup
	if( nodeIn->cb_auth == NULL ) return;

	// get our relevant information
	char* clientId;
	uint16_t clientIdLen_bytes;
	bool hasUsername;
	char* username = NULL;
	uint16_t usernameLen_bytes = 0;
	bool hasPassword;
	uint8_t* password = NULL;
	uint16_t passwordLen_bytes = 0;
	if( !cxa_mqtt_message_connect_getClientId(msgIn, &clientId, &clientIdLen_bytes) ||
			!cxa_mqtt_message_connect_hasUsername(msgIn, &hasUsername) ||
			(hasUsername && !cxa_mqtt_message_connect_getUsername(msgIn, &username, &usernameLen_bytes)) ||
			!cxa_mqtt_message_connect_hasPassword(msgIn, &hasPassword) ||
			(hasPassword && !cxa_mqtt_message_connect_getPassword(msgIn, &password, &passwordLen_bytes)) )
	{
		cxa_logger_warn(&nodeIn->super.logger, "problem getting CONNECT info");
		return;
	}

	// make sure our clientId is the appropriate size
	if( clientIdLen_bytes >= CXA_MQTT_RPC_NODE_BRIDGE_CLIENTID_MAXLEN_BYTES )
	{
		cxa_logger_warn(&nodeIn->super.logger, "clientId too long");
		sendMessage_connack(nodeIn, false, CXA_MQTT_CONNACK_RETCODE_REFUSED_CID);
		return;
	}

	// now, if we've already authorized this clientId we don't need to authorize
	cxa_array_iterate(&nodeIn->remoteNodes, currRemoteNode, cxa_mqtt_rpc_node_bridge_remoteNodeEntry_t)
	{
		if( currRemoteNode == NULL ) continue;

		if( clientIdLen_bytes == strlen(currRemoteNode->clientId) &&
				(memcmp(currRemoteNode->clientId, clientId, clientIdLen_bytes) == 0) )
		{
			// we have a match...send our CONNACK
			cxa_logger_debug(&nodeIn->super.logger, "CONNECT for prev auth client");
			sendMessage_connack(nodeIn, false, CXA_MQTT_CONNACK_RETCODE_ACCEPTED);
			return;
		}
	}

	// must be a new client...call our authentication function (already verified it was set)
	char* mappedName = nodeIn->cb_auth(clientId, clientIdLen_bytes, username, usernameLen_bytes, password, passwordLen_bytes, nodeIn->userVar_auth);
	if( mappedName == NULL )
	{
		cxa_logger_log_untermString(&nodeIn->super.logger, CXA_LOG_LEVEL_WARN, "client not authorized: '", clientId, clientIdLen_bytes, "'");
		sendMessage_connack(nodeIn, false, CXA_MQTT_CONNACK_RETCODE_REFUSED_BADUSERNAMEPASSWORD);
		return;
	}

	// if we made it here, we're authorized...add it to our map
	cxa_mqtt_rpc_node_bridge_remoteNodeEntry_t newEntry;
	memcpy(newEntry.clientId, clientId, clientIdLen_bytes);
	newEntry.clientId[clientIdLen_bytes] = 0;
	strlcpy(newEntry.mappedName, mappedName, sizeof(newEntry.mappedName));
	if( !cxa_array_append(&nodeIn->remoteNodes, &newEntry) )
	{
		cxa_logger_warn(&nodeIn->super.logger, "too many remote clients");
		sendMessage_connack(nodeIn, false, CXA_MQTT_CONNACK_RETCODE_REFUSED_SERVERUNAVAILABLE);
		return;
	}
	cxa_logger_info(&nodeIn->super.logger, "new client '%s'::'%s'", mappedName, newEntry.clientId);

	// send our connack
	sendMessage_connack(nodeIn, false, CXA_MQTT_CONNACK_RETCODE_ACCEPTED);
}


static void handleMessage_pingReq(cxa_mqtt_rpc_node_bridge_t *const nodeIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(nodeIn);

	cxa_mqtt_message_t* msg = NULL;
	if( ((msg = cxa_mqtt_messageFactory_getFreeMessage_empty()) == NULL) ||
			!cxa_mqtt_message_pingResponse_init(msg) ||
			!cxa_protocolParser_writePacket(&nodeIn->mpp.super, cxa_mqtt_message_getBuffer(msg)) )
	{
		cxa_logger_warn(&nodeIn->super.logger, "failed to send PINGRESP");
	}
	if( msg != NULL ) cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
}


static void handleMessage_subscribe(cxa_mqtt_rpc_node_bridge_t *const nodeIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(nodeIn);

	cxa_logger_trace(&nodeIn->super.logger, "got subscribe");
}


static void handleMessage_publish(cxa_mqtt_rpc_node_bridge_t *const nodeIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(nodeIn);

	cxa_logger_trace(&nodeIn->super.logger, "got publish");
}


static void sendMessage_connack(cxa_mqtt_rpc_node_bridge_t *const nodeIn, bool isSessionPresentIn, cxa_mqtt_connAck_returnCode_t retCodeIn)
{
	cxa_assert(nodeIn);

	cxa_mqtt_message_t* msg = NULL;
	if( ((msg = cxa_mqtt_messageFactory_getFreeMessage_empty()) == NULL) ||
			!cxa_mqtt_message_connack_init(msg, isSessionPresentIn, retCodeIn) ||
			!cxa_protocolParser_writePacket(&nodeIn->mpp.super, cxa_mqtt_message_getBuffer(msg)) )
	{
		cxa_logger_warn(&nodeIn->super.logger, "failed to send CONNACK");
	}
	if( msg != NULL ) cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
}


static bool rpcCb_catchall(cxa_mqtt_rpc_node_t *const superIn, char *const remainingTopicIn, size_t remainingTopicLen_bytes, cxa_mqtt_message_t *const msgIn, void* userVarIn)
{
	cxa_mqtt_rpc_node_bridge_t* nodeIn = (cxa_mqtt_rpc_node_bridge_t*)superIn;
	cxa_assert(nodeIn);

	cxa_logger_log_untermString(&nodeIn->super.logger, CXA_LOG_LEVEL_TRACE, "catchall: '", remainingTopicIn, remainingTopicLen_bytes, "'");

	// at this point, we should have a string something like 'foo/::bar'
	// iterate through our mapped nodes to see if 'foo' matches a node we know...
	cxa_array_iterate(&nodeIn->remoteNodes, currRemNode, cxa_mqtt_rpc_node_bridge_remoteNodeEntry_t)
	{
		if( currRemNode == NULL ) continue;

		size_t currMappedNameLen_bytes = strlen(currRemNode->mappedName);
		if( (currMappedNameLen_bytes <= remainingTopicLen_bytes) && cxa_stringUtils_startsWith(remainingTopicIn, currRemNode->mappedName) )
		{
			// we have a match!
			cxa_logger_trace(&nodeIn->super.logger, "found match: '%s'", currRemNode->mappedName);

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
				cxa_logger_warn(&nodeIn->super.logger, "error remapping topic name, dropping");
				return true;		// return true because we _should_ have handled this
			}

			// send the message
			if( !cxa_protocolParser_writePacket(&nodeIn->mpp.super, cxa_mqtt_message_getBuffer(msgIn)) )
			{
				cxa_logger_warn(&nodeIn->super.logger, "error forwarding message");
			}

			return true;
		}
	}

	// if we made it here, we found no matching remote nodes
	cxa_logger_log_untermString(&nodeIn->super.logger, CXA_LOG_LEVEL_WARN, "couldn't handle '", remainingTopicIn, remainingTopicLen_bytes, "'");
	return false;
}
