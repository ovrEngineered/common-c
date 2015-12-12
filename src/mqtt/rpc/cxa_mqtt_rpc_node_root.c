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
#include <cxa_stringUtils.h>
#include <cxa_assert.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define REQ_PREFIX					"::"
#define RESP_PREFIX					"/rpcResp"


// ******** local type definitions ********


// ******** local function prototypes ********
static void mqttClientCb_onPublish(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn,
									char* topicNameIn, size_t topicNameLen_bytesIn, void* payloadIn, size_t payloadLen_bytesIn, void* userVarIn);
static void sendResponse(cxa_mqtt_rpc_node_root_t *const nodeIn, char* topicNameIn, uint16_t idIn, cxa_mqtt_rpc_methodRetVal_t retValIn, cxa_fixedByteBuffer_t *const fbb_retParamsIn);


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

	// we can subscribe immediately because the mqtt client will cache subscribes if we're offline
	char subscriptTopic[CXA_MQTT_CLIENT_MAXLEN_TOPICFILTER_BYTES];
	subscriptTopic[0] = 0;
	if( rootPrefixIn != NULL )
	{
		cxa_assert( cxa_stringUtils_concat(subscriptTopic, rootPrefixIn, sizeof(subscriptTopic)) );
		cxa_assert( cxa_stringUtils_concat(subscriptTopic, "/", sizeof(subscriptTopic)) );
	}
	cxa_assert( cxa_stringUtils_concat(subscriptTopic, nodeIn->super.name, sizeof(subscriptTopic)) );
	cxa_assert( cxa_stringUtils_concat(subscriptTopic, "/#", sizeof(subscriptTopic)) );

	// save our prefix length (+1 is for path separator)
	nodeIn->prefixLen_bytes = (rootPrefixIn == NULL) ? 0 : strlen(rootPrefixIn)+1;

	cxa_mqtt_client_subscribe(nodeIn->mqttClient, subscriptTopic, CXA_MQTT_QOS_ATMOST_ONCE, mqttClientCb_onPublish, (void*)nodeIn);
}


cxa_mqtt_client_t* cxa_mqtt_rpc_node_root_getMqttClient(cxa_mqtt_rpc_node_root_t *const nodeIn)
{
	cxa_assert(nodeIn);

	return nodeIn->mqttClient;
}


// ******** local function implementations ********
static void mqttClientCb_onPublish(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn,
									char* topicNameIn, size_t topicNameLen_bytesIn, void* payloadIn, size_t payloadLen_bytesIn, void* userVarIn)
{
	cxa_assert(topicNameIn);
	cxa_mqtt_rpc_node_root_t* nodeIn = (cxa_mqtt_rpc_node_root_t*)userVarIn;
	cxa_assert(nodeIn);

	// this is probably a good point to get the id of the message
	cxa_fixedByteBuffer_t fbb_params;
	uint16_t id;
	if( (payloadIn == NULL) || (payloadLen_bytesIn < 2) || (topicNameLen_bytesIn < nodeIn->prefixLen_bytes) )
	{
		cxa_logger_warn(&nodeIn->super.logger, "malformed request: %p %d", payloadIn, payloadLen_bytesIn);
		return;
	}
	cxa_fixedByteBuffer_init_inPlace(&fbb_params, payloadLen_bytesIn, payloadIn, payloadLen_bytesIn);
	cxa_fixedByteBuffer_get_uint16BE(&fbb_params, 0, id);
	bool hasParams = false;
	if( payloadLen_bytesIn > 2 )
	{
		// re-initialize our parameter buffer to exclude the ID
		cxa_fixedByteBuffer_init_inPlace(&fbb_params, payloadLen_bytesIn-2, &(((uint8_t*)payloadIn)[2]), payloadLen_bytesIn-2);
		hasParams = true;
	}

	// wipe our prefix from the incoming topic
	topicNameIn += nodeIn->prefixLen_bytes;
	topicNameLen_bytesIn -= nodeIn->prefixLen_bytes;

	// after this point, we'll be sending a response...so get it ready
	// our return parameters +1 byte for return success
	cxa_fixedByteBuffer_t fbb_retParams;
	uint8_t fbb_retParams_raw[CXA_MQTT_RPCNODE_MAXLEN_RETURNPARAMS_BYTES];
	cxa_fixedByteBuffer_initStd(&fbb_retParams, fbb_retParams_raw);


	// remove ourselves from the path (ie. /dev/serialNUM/)
	cxa_mqtt_rpc_node_t *currNode = &nodeIn->super;
	char* currPath = topicNameIn;
	size_t currPathLen_bytes = topicNameLen_bytesIn;
	while( currPathLen_bytes > 0 )
	{
		size_t currNodeNameLen_bytes = strlen(currNode->name);
		if( (currNodeNameLen_bytes > currPathLen_bytes) || !cxa_stringUtils_startsWith(currPath, currNode->name) )
		{
			cxa_logger_log_untermString(&nodeIn->super.logger, CXA_LOG_LEVEL_WARN, "unknown node '", currPath, currPathLen_bytes, "'");
			sendResponse(nodeIn, topicNameIn, id, CXA_MQTT_RPC_METHODRETVAL_FAIL_MALFORMED_PATH, &fbb_retParams);
			return;
		}
		// we know that the currPath starts with our node name...move our path forward
		currPath += currNodeNameLen_bytes;
		currPathLen_bytes -= currNodeNameLen_bytes;

		// currPath should start with a path separator at this point
		if( (currPathLen_bytes < 1) || (*currPath != '/') )
		{
			cxa_logger_log_untermString(&nodeIn->super.logger, CXA_LOG_LEVEL_WARN, "malformed path '", currPath, currPathLen_bytes, "'");
			sendResponse(nodeIn, topicNameIn, id, CXA_MQTT_RPC_METHODRETVAL_FAIL_MALFORMED_PATH, &fbb_retParams);
			return;
		}
		currPath++;
		currPathLen_bytes--;

		// once we've reached here, we should either be a subNode name or a method name
		if( (currPathLen_bytes > strlen(REQ_PREFIX)) && cxa_stringUtils_startsWith(currPath, REQ_PREFIX) )
		{
			// move the path forward
			currPath += strlen(REQ_PREFIX);
			currPathLen_bytes -= strlen(REQ_PREFIX);

			// start looking for a method
			cxa_array_iterate(&currNode->methods, currMethodEntry, cxa_mqtt_rpc_node_methodEntry_t)
			{
				if( currMethodEntry == NULL ) continue;

				size_t currMethodNameLen_bytes = strlen(currMethodEntry->name);
				if( (currMethodNameLen_bytes == currPathLen_bytes) && strcmp(currMethodEntry->name, currPath) )
				{
					cxa_logger_trace(&nodeIn->super.logger, "found method '%s'", currMethodEntry->name);
					if( currMethodEntry->cb_method != NULL ) currMethodEntry->cb_method(currNode, &fbb_params, &fbb_retParams, currMethodEntry->userVar);
					return;
				}
			}

			// if we made it here, we couldn't find the right method...see if we have a catchall
			if( currNode->cb_catchall != NULL )
			{
				// only return if the catchall was successful
				char* fullMethod = currPath - strlen(REQ_PREFIX);
				size_t fullMethodLen_bytes = currPathLen_bytes + strlen(REQ_PREFIX);

				if( currNode->cb_catchall(currNode, fullMethod, fullMethodLen_bytes, msgIn, currNode->catchAll_userVar) ) return;
			}

			// if we made it here, we coudn't find the right method
			cxa_logger_log_untermString(&nodeIn->super.logger, CXA_LOG_LEVEL_WARN, "unknown method: '", currPath, currPathLen_bytes, "'");
			sendResponse(nodeIn, topicNameIn, id, CXA_MQTT_RPC_METHODRETVAL_FAIL_METHOD_DNE, &fbb_retParams);
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
			sendResponse(nodeIn, topicNameIn, id, CXA_MQTT_RPC_METHODRETVAL_FAIL_NODE_DNE, &fbb_retParams);
			return;
		}
	}
}


static void sendResponse(cxa_mqtt_rpc_node_root_t *const nodeIn, char* topicNameIn, uint16_t idIn, cxa_mqtt_rpc_methodRetVal_t retValIn, cxa_fixedByteBuffer_t *const fbb_retParamsIn)
{
	cxa_assert(nodeIn);
	cxa_assert(fbb_retParamsIn);

	char respTopic[CXA_MQTT_CLIENT_MAXLEN_TOPICFILTER_BYTES+1] = RESP_PREFIX;
	char id_str[9];
	sprintf(id_str, "/%d/%d", idIn, retValIn);
	if( !cxa_stringUtils_concat(respTopic, topicNameIn, sizeof(respTopic)) ||
		!cxa_stringUtils_concat(respTopic, id_str, sizeof(respTopic)) )
	{
		cxa_logger_warn(&nodeIn->super.logger, "problem assembling response");
		return;
	}

	cxa_mqtt_client_publish(nodeIn->mqttClient, CXA_MQTT_QOS_ATMOST_ONCE, false, respTopic,
							cxa_fixedByteBuffer_get_pointerToIndex(fbb_retParamsIn, 0),
							cxa_fixedByteBuffer_getSize_bytes(fbb_retParamsIn));
}
