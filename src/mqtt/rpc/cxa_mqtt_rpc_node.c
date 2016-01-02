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
#include <cxa_mqtt_rpc_node_root.h>
#include <cxa_stringUtils.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static bool getTopicForNodeAndNotification(cxa_mqtt_rpc_node_t *const nodeIn, char *const notiNameIn, char* topicOut, size_t maxTopicLen_bytesIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_mqtt_rpc_node_vinit(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn, const char *nameFmtIn, va_list varArgsIn)
{
	cxa_assert(nodeIn);
	cxa_assert(nameFmtIn);

	// save our references and set some defaults
	nodeIn->parentNode = parentNodeIn;
	nodeIn->isRootNode = false;

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


bool cxa_mqtt_rpc_node_executeMethod(cxa_mqtt_rpc_node_t *const nodeIn, char *const methodNameIn, char *const pathToNodeIn, cxa_fixedByteBuffer_t *const paramsIn)
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
	snprintf(msgId, sizeof(msgId), "%04X", currRequestId++);
	msgId[4] = 0;
	if( !cxa_mqtt_message_publish_topicName_prependCString(msg, msgId) ||
		!cxa_mqtt_message_publish_topicName_prependCString(msg, "/") )
	{
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}

	// now the method name
	if( !cxa_mqtt_message_publish_topicName_prependCString(msg, methodNameIn) ||
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

	// excellent...now we need to figure out where this message is headed...
	if( cxa_stringUtils_startsWith(pathToNodeIn, "/") )
	{
		// this message is addressed from the global root...send it up!
		cxa_mqtt_rpc_node_root_t* rootNode = cxa_mqtt_rpc_node_getRootNode(nodeIn);
		if( rootNode == NULL )
		{
			cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
			return false;
		}
		cxa_mqtt_rpc_node_root_handleInternalPublish(rootNode, msg);
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return true;
	}

	// if we made it here, this message must be headed for one of _our_ subnodes...


	return true;
}


bool cxa_mqtt_rpc_node_publishNotification(cxa_mqtt_rpc_node_t *const nodeIn, char *const notiNameIn, cxa_mqtt_qosLevel_t qosIn, void* dataIn, size_t dataSize_bytesIn)
{
	cxa_assert(nodeIn);
	cxa_assert(notiNameIn);

	return false;
}


void cxa_mqtt_rpc_node_setCatchAll(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_cb_catchall_t cb_catchallIn, void *userVarIn)
{
	cxa_assert(nodeIn);

	nodeIn->cb_catchall = cb_catchallIn;
	nodeIn->catchAll_userVar = userVarIn;
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


cxa_mqtt_rpc_node_root_t* cxa_mqtt_rpc_node_getRootNode(cxa_mqtt_rpc_node_t *const nodeIn)
{
	cxa_assert(nodeIn);

	cxa_mqtt_rpc_node_t* currNode = nodeIn;
	while( currNode != NULL )
	{
		if( currNode->isRootNode ) return (cxa_mqtt_rpc_node_root_t*)currNode;

		currNode = currNode->parentNode;
	}

	return NULL;
}


// ******** local function implementations ********
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
