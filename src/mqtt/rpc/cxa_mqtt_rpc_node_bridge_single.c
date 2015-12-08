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
#include <cxa_stringUtils.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static char* bridgeAuthCb(char *const clientIdIn, size_t clientIdLen_bytes,
							char *const usernameIn, size_t usernameLen_bytesIn,
							uint8_t *const passwordIn, size_t passwordLen_bytesIn,
							void *userVarIn);

static bool rpcCb_catchall(cxa_mqtt_rpc_node_t *const superIn, char *const remainingTopicIn, size_t remainingTopicLen_bytes, cxa_mqtt_message_t *const msgIn, void* userVarIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_mqtt_rpc_node_bridge_single_init(cxa_mqtt_rpc_node_bridge_single_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn,
										 cxa_ioStream_t *const iosIn, cxa_timeBase_t *const timeBaseIn, const char *nameFmtIn, ...)
{
	cxa_assert(nodeIn);
	cxa_assert(parentNodeIn);
	cxa_assert(iosIn);
	cxa_assert(timeBaseIn);
	cxa_assert(nameFmtIn);

	// initialize our super class
	va_list varArgs;
	va_start(varArgs, nameFmtIn);
	cxa_mqtt_rpc_node_bridge_vinit(&nodeIn->super, parentNodeIn, iosIn, timeBaseIn, bridgeAuthCb, (void*)nodeIn, nameFmtIn, varArgs);
	va_end(varArgs);
	cxa_mqtt_rpc_node_setCatchAll(&nodeIn->super.super, rpcCb_catchall, (void*)nodeIn);

	// set some defaults
	nodeIn->clientId[0] = 0;
	nodeIn->hasClientAuthed = false;
}


// ******** local function implementations ********
static char* bridgeAuthCb(char *const clientIdIn, size_t clientIdLen_bytes,
							char *const usernameIn, size_t usernameLen_bytesIn,
							uint8_t *const passwordIn, size_t passwordLen_bytesIn,
							void *userVarIn)
{
	cxa_mqtt_rpc_node_bridge_single_t* nodeIn = (cxa_mqtt_rpc_node_bridge_single_t*)userVarIn;
	cxa_assert(nodeIn);

	cxa_logger_info(&nodeIn->super.super.logger, "remote client connected");

	// make sure the client ID is an appropriate length
	if( clientIdLen_bytes > (sizeof(nodeIn->clientId)-1) )
	{
		cxa_logger_warn(&nodeIn->super.super.logger, "remote clientId too long");
		return NULL;
	}

	// store a local copy of the clientID (null terminated)
	memcpy(nodeIn->clientId, clientIdIn, clientIdLen_bytes);
	nodeIn->clientId[clientIdLen_bytes] = 0;

	nodeIn->hasClientAuthed = true;

	return nodeIn->super.super.name;
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
		!cxa_mqtt_message_publish_topicName_prependCString(msgIn, nodeIn->clientId) ||
		!cxa_mqtt_message_publish_topicName_prependCString(msgIn, "~/") )
	{
		cxa_logger_warn(&nodeIn->super.super.logger, "error remapping topic name, dropping");
		return true;		// return true because we _should_ have handled this
	}

	// send the message
	if( !cxa_protocolParser_writePacket(&nodeIn->super.mpp.super, cxa_mqtt_message_getBuffer(msgIn)) )
	{
		cxa_logger_warn(&nodeIn->super.super.logger, "error forwarding message");
	}
	return true;
}
