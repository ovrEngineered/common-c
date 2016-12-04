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
#include <cxa_mqtt_rpc_message.h>
#include <cxa_stringUtils.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void mqttClientCb_onConnect(cxa_mqtt_client_t *const clientIn, void* userVarIn);
static void mqttClientCb_onPublish(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn,
									char* topicNameIn, size_t topicNameLen_bytesIn, void* payloadIn, size_t payloadLen_bytesIn, void* userVarIn);

static void scm_handleMessage_upstream(cxa_mqtt_rpc_node_t *const superIn, cxa_mqtt_message_t *const msgIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_mqtt_rpc_node_root_init(cxa_mqtt_rpc_node_root_t *const nodeIn, cxa_mqtt_client_t* const clientIn,
								 bool reportStateIn, const char *nameFmtIn, ...)
{
	cxa_assert(nodeIn);
	cxa_assert(nameFmtIn);
	cxa_assert(clientIn);

	// save our references
	nodeIn->mqttClient = clientIn;
	nodeIn->currRequestId = 0;
	nodeIn->shouldReportState = reportStateIn;

	// initialize our super class
	va_list varArgs;
	va_start(varArgs, nameFmtIn);
	cxa_mqtt_rpc_node_vinit(&nodeIn->super, NULL, nameFmtIn, varArgs);
	va_end(varArgs);

	// setup our subclass methods / overrides
	nodeIn->super.scm_handleMessage_upstream = scm_handleMessage_upstream;

	// set our last-will-testament message (for status)
	char stateTopic[CXA_MQTT_CLIENT_MAXLEN_TOPICFILTER_BYTES];
	stateTopic[0] = 0;
	cxa_assert( cxa_stringUtils_concat(stateTopic, nodeIn->super.name, sizeof(stateTopic)) );
	cxa_assert( cxa_stringUtils_concat(stateTopic, "/" CXA_MQTT_RPCNODE_NOTI_PREFIX CXA_MQTT_RPCNODE_CONNSTATE_TOPIC, sizeof(stateTopic)) );
	#ifdef CXA_MQTTSERVER_ISAWS
		cxa_assert(cxa_mqtt_client_setWillMessage(nodeIn->mqttClient,
												  CXA_MQTT_QOS_ATMOST_ONCE,
												  false,
												  stateTopic,
												  "{\"state\":{\"reported\":{\"connState\":0}}}", 38));
	#else
		cxa_assert(cxa_mqtt_client_setWillMessage(nodeIn->mqttClient,
												  CXA_MQTT_QOS_ATMOST_ONCE,
												  true,
												  stateTopic,
												  ((uint8_t[]){0x00}), 1));
	#endif

	// we can subscribe immediately because the mqtt client will cache subscribes if we're offline
	char subscriptTopic[CXA_MQTT_CLIENT_MAXLEN_TOPICFILTER_BYTES];
	subscriptTopic[0] = 0;
	cxa_assert( cxa_stringUtils_concat(subscriptTopic, nodeIn->super.name, sizeof(subscriptTopic)) );
	cxa_assert( cxa_stringUtils_concat(subscriptTopic, "/#", sizeof(subscriptTopic)) );

	// register for mqtt events
	cxa_mqtt_client_addListener(nodeIn->mqttClient, mqttClientCb_onConnect, NULL, NULL, (void*)nodeIn);
	cxa_mqtt_client_subscribe(nodeIn->mqttClient, subscriptTopic, CXA_MQTT_QOS_ATMOST_ONCE, mqttClientCb_onPublish, (void*)nodeIn);
}


// ******** local function implementations ********
static void mqttClientCb_onConnect(cxa_mqtt_client_t *const clientIn, void* userVarIn)
{
	cxa_mqtt_rpc_node_root_t* nodeIn = (cxa_mqtt_rpc_node_root_t*)userVarIn;
	cxa_assert(nodeIn);

	if( !nodeIn->shouldReportState ) return;

	// publish our state
	char stateTopic[CXA_MQTT_CLIENT_MAXLEN_TOPICFILTER_BYTES];
	stateTopic[0] = 0;
	cxa_assert( cxa_stringUtils_concat(stateTopic, nodeIn->super.name, sizeof(stateTopic)) );
	cxa_assert( cxa_stringUtils_concat(stateTopic, "/" CXA_MQTT_RPCNODE_NOTI_PREFIX CXA_MQTT_RPCNODE_CONNSTATE_TOPIC, sizeof(stateTopic)) );
	#ifdef CXA_MQTTSERVER_ISAWS
		cxa_mqtt_client_publish(clientIn,
								CXA_MQTT_QOS_ATMOST_ONCE,
								false,
								stateTopic,
								"{\"state\":{\"reported\":{\"connState\":1}}}", 38);
	#else
		cxa_mqtt_client_publish(clientIn,
								CXA_MQTT_QOS_ATMOST_ONCE,
								true,
								stateTopic,
								((uint8_t[]){0x01}), 1);
	#endif
}


static void mqttClientCb_onPublish(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn,
									char* topicNameIn, size_t topicNameLen_bytesIn, void* payloadIn, size_t payloadLen_bytesIn, void* userVarIn)
{
	cxa_assert(topicNameIn);
	cxa_mqtt_rpc_node_root_t* nodeIn = (cxa_mqtt_rpc_node_root_t*)userVarIn;
	cxa_assert(nodeIn);

	// insert it into our message handling pipeline
	nodeIn->super.scm_handleMessage_downstream(&nodeIn->super, topicNameIn, topicNameLen_bytesIn, msgIn);
}


static void scm_handleMessage_upstream(cxa_mqtt_rpc_node_t *const superIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_mqtt_rpc_node_root_t* nodeIn = (cxa_mqtt_rpc_node_root_t*)superIn;
	cxa_assert(nodeIn);

	char* topicName;
	uint16_t topicNameLen_bytes;
	if( !cxa_mqtt_message_publish_getTopicName(msgIn, &topicName, &topicNameLen_bytes) ) return;

	cxa_logger_log_untermString(&superIn->logger, CXA_LOG_LEVEL_TRACE, "<< '", topicName, topicNameLen_bytes, "'");

	if( cxa_stringUtils_startsWith_withLengths(topicName, topicNameLen_bytes, CXA_MQTT_RPCNODE_LOCALROOT_PREFIX, strlen(CXA_MQTT_RPCNODE_LOCALROOT_PREFIX)) )
	{
		// it's a local request/response/notification and we've reached the top of our hierarchy...start passing it down
		if( superIn->scm_handleMessage_downstream(superIn, topicName, topicNameLen_bytes, msgIn) ) return;

		// if we made it here we couldn't find anything that would handle this message...remap it and toss it up to the server
		if( !cxa_mqtt_message_publish_topicName_trimToPointer(msgIn, topicName+strlen(CXA_MQTT_RPCNODE_LOCALROOT_PREFIX)) ||
			!cxa_mqtt_message_publish_topicName_prependCString(msgIn, "/") ||
			!cxa_mqtt_message_publish_topicName_prependCString(msgIn, nodeIn->super.name) ) return;
	}

	// if we made it here, forward this message up to the cloud
	if( (msgIn != NULL) && cxa_mqtt_client_isConnected(nodeIn->mqttClient) )
	{
		cxa_mqtt_client_publish_message(nodeIn->mqttClient, msgIn);
	}
}
