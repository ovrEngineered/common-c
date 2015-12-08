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
#include <cxa_mqtt_rpc_node_root.h>
#include <cxa_stringUtils.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static cxa_mqtt_client_t* getMqttClient(cxa_mqtt_rpc_node_t *const nodeIn);
static bool getTopicForNodeAndNotification(cxa_mqtt_rpc_node_t *const nodeIn, char *const notiNameIn, char* topicOut, size_t maxTopicLen_bytesIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_mqtt_rpc_node_vinit(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn, const char *nameFmtIn, va_list varArgsIn)
{
	cxa_assert(nodeIn);
	cxa_assert(nameFmtIn);

	// save our references
	nodeIn->parentNode = parentNodeIn;

	// assemble our name
	vsnprintf(nodeIn->name, CXA_MQTT_RPCNODE_MAXLEN_NAME_BYTES, nameFmtIn, varArgsIn);
	nodeIn->name[CXA_MQTT_RPCNODE_MAXLEN_NAME_BYTES-1] = 0;

	// setup our subnodes, methods, catchalls
	cxa_array_initStd(&nodeIn->subNodes, nodeIn->subNodes_raw);
	cxa_array_initStd(&nodeIn->methods, nodeIn->methods_raw);
	nodeIn->cb_catchall = NULL;
	nodeIn->catchAll_userVar = NULL;

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


void cxa_mqtt_rpc_node_setCatchAll(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_cb_catchall_t cb_catchallIn, void *userVarIn)
{
	cxa_assert(nodeIn);

	nodeIn->cb_catchall = cb_catchallIn;
	nodeIn->catchAll_userVar = userVarIn;
}


bool cxa_mqtt_rpc_node_publishNotification(cxa_mqtt_rpc_node_t *const nodeIn, char *const notiNameIn, cxa_mqtt_qosLevel_t qosIn, void* dataIn, size_t dataSize_bytesIn)
{
	cxa_assert(nodeIn);
	cxa_assert(notiNameIn);

	// get our mqtt client first
	cxa_mqtt_client_t* mqttClient = getMqttClient(nodeIn);

	// now get our publish topic and publish
	char publishTopic[CXA_MQTT_CLIENT_MAXLEN_TOPICFILTER_BYTES];
	if( (mqttClient == NULL) ||
			!getTopicForNodeAndNotification(nodeIn, "temp_c", publishTopic, sizeof(publishTopic)) ||
			!cxa_mqtt_client_publish(mqttClient, CXA_MQTT_QOS_ATMOST_ONCE, false, publishTopic, dataIn, dataSize_bytesIn) )
	{
		cxa_logger_warn(&nodeIn->logger, "error publishing notification '%s'", notiNameIn);
		return false;
	}

	return true;
}


bool cxa_mqtt_rpc_node_getTopicForNode(cxa_mqtt_rpc_node_t *const nodeIn, char* topicOut, size_t maxTopicLen_bytesIn)
{
	cxa_assert(nodeIn);
	cxa_assert(topicOut);

	// recurse to our parent first
	bool retVal = (nodeIn->parentNode != NULL) ? cxa_mqtt_rpc_node_getTopicForNode(nodeIn->parentNode, topicOut, maxTopicLen_bytesIn) : true;
	if( !retVal ) return false;

	// now do us (strlcat provides nice checks for us)

	// add a separator, if needed, then add our name
	if( (strlen(topicOut) > 0) && !cxa_stringUtils_concat(topicOut, "/", maxTopicLen_bytesIn) ) return false;
	if( !cxa_stringUtils_concat(topicOut, nodeIn->name, maxTopicLen_bytesIn) ) return false;

	// if we made it here, we're good to go!
	return true;
}


// ******** local function implementations ********
static cxa_mqtt_client_t* getMqttClient(cxa_mqtt_rpc_node_t *const nodeIn)
{
	cxa_assert(nodeIn);

	if( nodeIn->parentNode != NULL )
	{
		return getMqttClient(nodeIn->parentNode);
	}
	else
	{
		// this is _probably_ a root node
		return cxa_mqtt_rpc_node_root_getMqttClient((cxa_mqtt_rpc_node_root_t*)nodeIn);
	}

	return NULL;
}


static bool getTopicForNodeAndNotification(cxa_mqtt_rpc_node_t *const nodeIn, char *const notiNameIn, char* topicOut, size_t maxTopicLen_bytesIn)
{
	cxa_assert(nodeIn);
	cxa_assert(notiNameIn);
	cxa_assert(topicOut);

	if( !cxa_mqtt_rpc_node_getTopicForNode(nodeIn, topicOut, maxTopicLen_bytesIn) ) return false;

	// if we're the originator of this request, add the separator and wildcard
	if( !cxa_stringUtils_concat(topicOut, "/^^", maxTopicLen_bytesIn) ) return false;
	if( !cxa_stringUtils_concat(topicOut, notiNameIn, maxTopicLen_bytesIn) ) return false;

	// if we made it here, we're good to go!
	return true;
}
