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
#include "cxa_mqtt_rpc_node_root.h"


// ******** includes ********
#include <math.h>
#include <string.h>
#include <cxa_assert.h>
#include <cxa_mqtt_message_publish.h>
#include <cxa_stringUtils.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define CLIENT_STATE_TOPIC			"_state"


// ******** local type definitions ********


// ******** local function prototypes ********
static void mqttClientCb_onConnect(cxa_mqtt_client_t *const clientIn, void* userVarIn);
static void mqttClientCb_onPublish(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn,
									char* topicNameIn, size_t topicNameLen_bytesIn, void* payloadIn, size_t payloadLen_bytesIn, void* userVarIn);
static void sendResponse(cxa_mqtt_rpc_node_root_t *const nodeIn, char* topicNameIn, uint16_t topicLen_bytesIn, cxa_mqtt_rpc_methodRetVal_t retValIn, cxa_fixedByteBuffer_t *const fbb_retParamsIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_mqtt_rpc_node_root_init(cxa_mqtt_rpc_node_root_t *const nodeIn, cxa_mqtt_client_t* const clientIn, char *const rootPrefixIn, const char *nameFmtIn, ...)
{
	cxa_assert(nodeIn);
	cxa_assert(nameFmtIn);
	cxa_assert(clientIn);

	// save our references
	nodeIn->mqttClient = clientIn;

	// initialize our super class
	va_list varArgs;
	va_start(varArgs, nameFmtIn);
	cxa_mqtt_rpc_node_vinit(&nodeIn->super, NULL, nameFmtIn, varArgs);
	va_end(varArgs);
	nodeIn->super.isRootNode = true;

	// save our prefix and length (+1 is for path separator)
	nodeIn->prefix[0] = 0;
	if( rootPrefixIn != NULL )
	{
		cxa_assert( cxa_stringUtils_concat(nodeIn->prefix, rootPrefixIn, sizeof(nodeIn->prefix)) );
		cxa_assert( cxa_stringUtils_concat(nodeIn->prefix, "/", sizeof(nodeIn->prefix)) );
	}
	nodeIn->prefixLen_bytes = (rootPrefixIn == NULL) ? 0 : strlen(rootPrefixIn)+1;

	// set our last-will-testament message (for status)
	// publish our state
	char stateTopic[CXA_MQTT_CLIENT_MAXLEN_TOPICFILTER_BYTES];
	stateTopic[0] = 0;
	cxa_assert( cxa_stringUtils_concat(stateTopic, nodeIn->prefix, sizeof(stateTopic)) );
	cxa_assert( cxa_stringUtils_concat(stateTopic, nodeIn->super.name, sizeof(stateTopic)) );
	cxa_assert( cxa_stringUtils_concat(stateTopic, "/"CLIENT_STATE_TOPIC, sizeof(stateTopic)) );
	cxa_mqtt_client_setWillMessage(nodeIn->mqttClient, CXA_MQTT_QOS_ATMOST_ONCE, true, stateTopic, ((uint8_t[]){0x00}), 1);

	// we can subscribe immediately because the mqtt client will cache subscribes if we're offline
	char subscriptTopic[CXA_MQTT_CLIENT_MAXLEN_TOPICFILTER_BYTES];
	subscriptTopic[0] = 0;
	cxa_assert( cxa_stringUtils_concat(subscriptTopic, nodeIn->prefix, sizeof(subscriptTopic)) );
	cxa_assert( cxa_stringUtils_concat(subscriptTopic, nodeIn->super.name, sizeof(subscriptTopic)) );
	cxa_assert( cxa_stringUtils_concat(subscriptTopic, "/#", sizeof(subscriptTopic)) );

	// register for mqtt events
	cxa_mqtt_client_addListener(nodeIn->mqttClient, mqttClientCb_onConnect, NULL, NULL, (void*)nodeIn);
	cxa_mqtt_client_subscribe(nodeIn->mqttClient, subscriptTopic, CXA_MQTT_QOS_ATMOST_ONCE, mqttClientCb_onPublish, (void*)nodeIn);
}


cxa_mqtt_client_t* cxa_mqtt_rpc_node_root_getMqttClient(cxa_mqtt_rpc_node_root_t *const nodeIn)
{
	cxa_assert(nodeIn);

	return nodeIn->mqttClient;
}


char* cxa_mqtt_rpc_node_root_getPrefix(cxa_mqtt_rpc_node_root_t *const nodeIn)
{
	cxa_assert(nodeIn);

	return nodeIn->prefix;
}


void cxa_mqtt_rpc_node_root_handleInternalPublish(cxa_mqtt_rpc_node_root_t *const nodeIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(nodeIn);

	// get our topic name and length
	char* topicName;
	uint16_t topicLen_bytes;
	if( !cxa_mqtt_message_publish_getTopicName(msgIn, &topicName, &topicLen_bytes) ) return;

	// see if it's a response (if so, we need to do some extra processing)
	if( cxa_stringUtils_startsWith(topicName, CXA_MQTT_RPCNODE_RESP_PREFIX) )
	{
		// response...see if we have anyone locally looking for this response
		cxa_logger_log_untermString(&nodeIn->super.logger, CXA_LOG_LEVEL_TRACE, "got response '", topicName, topicLen_bytes, "'");

		// if we made it here, we don't have anyone locally looking for this response...
		// pass up through our client (maybe someone there is looking for it)
		cxa_logger_trace(&nodeIn->super.logger, "unknown response, forwarding");
		cxa_mqtt_client_publish_message(nodeIn->mqttClient, msgIn);
	}
}


// ******** local function implementations ********
static void mqttClientCb_onConnect(cxa_mqtt_client_t *const clientIn, void* userVarIn)
{
	cxa_mqtt_rpc_node_root_t* nodeIn = (cxa_mqtt_rpc_node_root_t*)userVarIn;
	cxa_assert(nodeIn);

	// publish our state
	char stateTopic[CXA_MQTT_CLIENT_MAXLEN_TOPICFILTER_BYTES];
	stateTopic[0] = 0;
	cxa_assert( cxa_stringUtils_concat(stateTopic, nodeIn->prefix, sizeof(stateTopic)) );
	cxa_assert( cxa_stringUtils_concat(stateTopic, nodeIn->super.name, sizeof(stateTopic)) );
	cxa_assert( cxa_stringUtils_concat(stateTopic, "/"CLIENT_STATE_TOPIC, sizeof(stateTopic)) );
	cxa_mqtt_client_publish(clientIn, CXA_MQTT_QOS_ATMOST_ONCE, true, stateTopic, ((uint8_t[]){0x01}), 1);
}


static void mqttClientCb_onPublish(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn,
									char* topicNameIn, size_t topicNameLen_bytesIn, void* payloadIn, size_t payloadLen_bytesIn, void* userVarIn)
{
	cxa_assert(topicNameIn);
	cxa_mqtt_rpc_node_root_t* nodeIn = (cxa_mqtt_rpc_node_root_t*)userVarIn;
	cxa_assert(nodeIn);

	// this _may_ be our client state message...if so, we don't need to do anything
	if( cxa_stringUtils_endsWith_withLengths(topicNameIn, topicNameLen_bytesIn, CLIENT_STATE_TOPIC) ) return;

	// this _may_ also be a response...if so, we don't need to do anything (for now)
	if( cxa_stringUtils_contains_withLengths(topicNameIn, topicNameLen_bytesIn, CXA_MQTT_RPCNODE_RESP_PREFIX, strlen(CXA_MQTT_RPCNODE_RESP_PREFIX)) ) return;

	// make sure our topic name is good
	if( topicNameLen_bytesIn < nodeIn->prefixLen_bytes )
	{
		cxa_logger_warn(&nodeIn->super.logger, "malformed request: %p %d", payloadIn, payloadLen_bytesIn);
		return;
	}

	// get our payload/params (if present)
	cxa_fixedByteBuffer_t fbb_params;
	bool hasParams = (payloadIn != NULL);
	if( hasParams ) cxa_fixedByteBuffer_init_inPlace(&fbb_params, payloadLen_bytesIn, payloadIn, payloadLen_bytesIn);

	// after this point, we'll be sending a response...so get it ready
	// our return parameters +1 byte for return success
	cxa_fixedByteBuffer_t fbb_retParams;
	uint8_t fbb_retParams_raw[CXA_MQTT_RPCNODE_MAXLEN_RETURNPARAMS_BYTES];
	cxa_fixedByteBuffer_initStd(&fbb_retParams, fbb_retParams_raw);


	// remove ourselves from the path (ie. /dev/serialNUM/)
	cxa_mqtt_rpc_node_t *currNode = &nodeIn->super;
	char* currPath = topicNameIn + nodeIn->prefixLen_bytes;
	size_t currPathLen_bytes = topicNameLen_bytesIn - nodeIn->prefixLen_bytes;
	while( currPathLen_bytes > 0 )
	{
		size_t currNodeNameLen_bytes = strlen(currNode->name);
		if( (currNodeNameLen_bytes > currPathLen_bytes) || !cxa_stringUtils_startsWith(currPath, currNode->name) )
		{
			cxa_logger_log_untermString(&nodeIn->super.logger, CXA_LOG_LEVEL_WARN, "unknown node '", currPath, currPathLen_bytes, "'");
			sendResponse(nodeIn, topicNameIn, topicNameLen_bytesIn, CXA_MQTT_RPC_METHODRETVAL_FAIL_MALFORMED_PATH, &fbb_retParams);
			return;
		}
		// we know that the currPath starts with our node name...move our path forward
		currPath += currNodeNameLen_bytes;
		currPathLen_bytes -= currNodeNameLen_bytes;

		// currPath should start with a path separator at this point
		if( (currPathLen_bytes < 1) || (*currPath != '/') )
		{
			cxa_logger_log_untermString(&nodeIn->super.logger, CXA_LOG_LEVEL_WARN, "malformed path '", currPath, currPathLen_bytes, "'");
			sendResponse(nodeIn, topicNameIn, topicNameLen_bytesIn, CXA_MQTT_RPC_METHODRETVAL_FAIL_MALFORMED_PATH, &fbb_retParams);
			return;
		}
		currPath++;
		currPathLen_bytes--;

		// once we've reached here, we should either be a subNode name or a method name
		if( (currPathLen_bytes > strlen(CXA_MQTT_RPCNODE_REQ_PREFIX)) && cxa_stringUtils_startsWith(currPath, CXA_MQTT_RPCNODE_REQ_PREFIX) )
		{
			// move the path forward
			currPath += strlen(CXA_MQTT_RPCNODE_REQ_PREFIX);
			currPathLen_bytes -= strlen(CXA_MQTT_RPCNODE_REQ_PREFIX);

			// start looking for a method
			cxa_array_iterate(&currNode->methods, currMethodEntry, cxa_mqtt_rpc_node_methodEntry_t)
			{
				if( currMethodEntry == NULL ) continue;

				size_t currMethodNameLen_bytes = strlen(currMethodEntry->name);
				if( (currMethodNameLen_bytes <= currPathLen_bytes) && cxa_stringUtils_startsWith(currPath, currMethodEntry->name) )
				{
					cxa_logger_trace(&nodeIn->super.logger, "found method '%s'", currMethodEntry->name);
					cxa_mqtt_rpc_methodRetVal_t retVal = CXA_MQTT_RPC_METHODRETVAL_SUCCESS;
					if( currMethodEntry->cb_method != NULL ) retVal = currMethodEntry->cb_method(currNode, (hasParams ? &fbb_params : NULL), &fbb_retParams, currMethodEntry->userVar);
					sendResponse(nodeIn, topicNameIn, topicNameLen_bytesIn, retVal, &fbb_retParams);
					return;
				}
			}

			// if we made it here, we couldn't find the right method...see if we have a catchall
			if( currNode->cb_catchall != NULL )
			{
				// only return if the catchall was successful
				char* fullMethod = currPath - strlen(CXA_MQTT_RPCNODE_REQ_PREFIX);
				size_t fullMethodLen_bytes = currPathLen_bytes + strlen(CXA_MQTT_RPCNODE_REQ_PREFIX);

				if( currNode->cb_catchall(currNode, fullMethod, fullMethodLen_bytes, msgIn, currNode->catchAll_userVar) ) return;
			}

			// if we made it here, we coudn't find the right method
			cxa_logger_log_untermString(&nodeIn->super.logger, CXA_LOG_LEVEL_WARN, "unknown method: '", currPath, currPathLen_bytes, "'");
			sendResponse(nodeIn, topicNameIn, topicNameLen_bytesIn, CXA_MQTT_RPC_METHODRETVAL_FAIL_METHOD_DNE, &fbb_retParams);
			return;
		}
		else
		{
			// start looking for a subnode
			bool continueToNextPathComponent = false;
			cxa_array_iterate(&currNode->subNodes, currSubNode, cxa_mqtt_rpc_node_t*)
			{
				if( (currSubNode == NULL) ) continue;

				size_t currSubNodeNameLen_bytes = strlen((*currSubNode)->name);
				if( (currSubNodeNameLen_bytes <= currPathLen_bytes) && cxa_stringUtils_startsWith(currPath, (*currSubNode)->name) )
				{
					currNode = *currSubNode;
					continueToNextPathComponent = true;
					break;
				}
			}
			if( continueToNextPathComponent ) continue;

			// if we made it here, we couldn't find the right subnode...see if we have a catchall
			if( currNode->cb_catchall != NULL )
			{
				// only return if the catchall was successful
				if( currNode->cb_catchall(currNode, currPath, currPathLen_bytes, msgIn, currNode->catchAll_userVar) ) return;
			}

			// no catchall (or it wasn't successful)...error
			cxa_logger_log_untermString(&nodeIn->super.logger, CXA_LOG_LEVEL_WARN, "unknown node '", currPath, currPathLen_bytes, "'");
			sendResponse(nodeIn, topicNameIn, topicNameLen_bytesIn, CXA_MQTT_RPC_METHODRETVAL_FAIL_NODE_DNE, &fbb_retParams);
			return;
		}
	}
}


static void sendResponse(cxa_mqtt_rpc_node_root_t *const nodeIn, char* topicNameIn, uint16_t topicLen_bytesIn, cxa_mqtt_rpc_methodRetVal_t retValIn, cxa_fixedByteBuffer_t *const fbb_retParamsIn)
{
	cxa_assert(nodeIn);
	cxa_assert(fbb_retParamsIn);

	// modify our response topic and prepend our retVal to the return params
	char respTopic[CXA_MQTT_CLIENT_MAXLEN_TOPICFILTER_BYTES+1] = { 0 };
	if( !cxa_stringUtils_concat_withLengths(respTopic, sizeof(respTopic), topicNameIn, topicLen_bytesIn) ||
		!cxa_stringUtils_replaceFirstOccurance(respTopic, CXA_MQTT_RPCNODE_REQ_PREFIX, CXA_MQTT_RPCNODE_RESP_PREFIX) ||
		!cxa_fixedByteBuffer_insert_uint8(fbb_retParamsIn, 0, (uint8_t)retValIn) )
	{
		cxa_logger_warn(&nodeIn->super.logger, "problem assembling response");
		return;
	}

	// send the response
	cxa_mqtt_client_publish(nodeIn->mqttClient, CXA_MQTT_QOS_ATMOST_ONCE, false, respTopic,
							cxa_fixedByteBuffer_get_pointerToIndex(fbb_retParamsIn, 0),
							cxa_fixedByteBuffer_getSize_bytes(fbb_retParamsIn));
}
