/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
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

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_INFO
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define CONN_STATE_PAYLOAD_CONN			"{\"value_num\":1}"
#define CONN_STATE_PAYLOAD_DISCONN		"{\"value_num\":0}"


// ******** local type definitions ********


// ******** local function prototypes ********
static void mqttClientCb_onConnect(cxa_mqtt_client_t *const clientIn, void* userVarIn);
static void mqttClientCb_onPublish(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn,
									char* topicNameIn, size_t topicNameLen_bytesIn, void* payloadIn, size_t payloadLen_bytesIn, void* userVarIn);

static void scm_handleMessage_upstream(cxa_mqtt_rpc_node_t *const superIn, cxa_mqtt_message_t *const msgIn);
static cxa_mqtt_client_t* scm_getClient(cxa_mqtt_rpc_node_t *const superIn);

static cxa_mqtt_rpc_methodRetVal_t rpcMethodCb_isAlive(cxa_mqtt_rpc_node_t *const superIn,
													   cxa_linkedField_t *const paramsIn, cxa_linkedField_t *const returnParamsOut,
													   void* userVarIn);


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
	cxa_mqtt_rpc_node_vinit2(&nodeIn->super, NULL,
							scm_handleMessage_upstream, NULL, scm_getClient,
							nameFmtIn, varArgs);
	va_end(varArgs);

	// set our last-will-testament message (for status)
	// v1/^^/30:AE:A4:01:74:90/streams/amb_temp_ddc/onStreamUpdate
	char stateTopic[CXA_MQTT_CLIENT_MAXLEN_TOPICFILTER_BYTES];
	stateTopic[0] = 0;
	cxa_assert( cxa_stringUtils_concat(stateTopic, CXA_MQTT_RPC_MESSAGE_VERSION "/^^/", sizeof(stateTopic)) );
	cxa_assert( cxa_stringUtils_concat(stateTopic, nodeIn->super.name, sizeof(stateTopic)) );
	cxa_assert( cxa_stringUtils_concat(stateTopic, "/streams/" CXA_MQTT_RPCNODE_CONNSTATE_STREAM_NAME "/onStreamUpdate", sizeof(stateTopic)) );
	cxa_assert(cxa_mqtt_client_setWillMessage(nodeIn->mqttClient,
											  CXA_MQTT_QOS_ATMOST_ONCE,
											  false,
											  stateTopic,
											  CONN_STATE_PAYLOAD_DISCONN, strlen(CONN_STATE_PAYLOAD_DISCONN)));

	// we can subscribe immediately because the mqtt client will cache subscribes if we're offline
	char subscriptTopic[CXA_MQTT_CLIENT_MAXLEN_TOPICFILTER_BYTES];
	subscriptTopic[0] = 0;
	cxa_assert( cxa_stringUtils_concat(subscriptTopic, CXA_MQTT_RPC_MESSAGE_VERSION "/->/", sizeof(subscriptTopic)) );
	cxa_assert( cxa_stringUtils_concat(subscriptTopic, nodeIn->super.name, sizeof(subscriptTopic)) );
	cxa_assert( cxa_stringUtils_concat(subscriptTopic, "/#", sizeof(subscriptTopic)) );
	cxa_mqtt_client_subscribe(nodeIn->mqttClient, subscriptTopic, CXA_MQTT_QOS_ATMOST_ONCE, mqttClientCb_onPublish, (void*)nodeIn);

	// register for MQTT events
	cxa_mqtt_client_addListener(nodeIn->mqttClient, mqttClientCb_onConnect, NULL, NULL, NULL,  (void*)nodeIn);

	cxa_mqtt_rpc_node_addMethod(&nodeIn->super, "isAlive", rpcMethodCb_isAlive, (void*)nodeIn);
}


// ******** local function implementations ********
static void mqttClientCb_onConnect(cxa_mqtt_client_t *const clientIn, void* userVarIn)
{
	cxa_mqtt_rpc_node_root_t* nodeIn = (cxa_mqtt_rpc_node_root_t*)userVarIn;
	cxa_assert(nodeIn);

	if( !nodeIn->shouldReportState ) return;

	// publish our connection state
	// v1/^^/30:AE:A4:01:74:90/streams/amb_temp_ddc/onStreamUpdate
	char stateTopic[CXA_MQTT_CLIENT_MAXLEN_TOPICFILTER_BYTES];
	stateTopic[0] = 0;
	cxa_assert( cxa_stringUtils_concat(stateTopic, "v1/^^/", sizeof(stateTopic)) );
	cxa_assert( cxa_stringUtils_concat(stateTopic, nodeIn->super.name, sizeof(stateTopic)) );
	cxa_assert( cxa_stringUtils_concat(stateTopic, "/streams/" CXA_MQTT_RPCNODE_CONNSTATE_STREAM_NAME "/onStreamUpdate", sizeof(stateTopic)) );
	cxa_mqtt_client_publish(clientIn,
							CXA_MQTT_QOS_ATMOST_ONCE,
							false,
							stateTopic,
							CONN_STATE_PAYLOAD_CONN, strlen(CONN_STATE_PAYLOAD_CONN));
}


static void mqttClientCb_onPublish(cxa_mqtt_client_t *const clientIn, cxa_mqtt_message_t *const msgIn,
									char* topicNameIn, size_t topicNameLen_bytesIn, void* payloadIn, size_t payloadLen_bytesIn, void* userVarIn)
{
	cxa_assert(topicNameIn);
	cxa_mqtt_rpc_node_root_t* nodeIn = (cxa_mqtt_rpc_node_root_t*)userVarIn;
	cxa_assert(nodeIn);

	// remove the version and type information
	size_t minMessageLen = strlen(CXA_MQTT_RPC_MESSAGE_VERSION "/" CXA_MQTT_RPCNODE_REQ_PREFIX "/");
	if( topicNameLen_bytesIn < minMessageLen ) return;
	topicNameIn += minMessageLen;
	topicNameLen_bytesIn -= minMessageLen;

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

//	cxa_logger_log_untermString(&superIn->logger, CXA_LOG_LEVEL_TRACE, "<< '", topicName, topicNameLen_bytes, "'");

	if( cxa_stringUtils_startsWith_withLengths(topicName, topicNameLen_bytes, CXA_MQTT_RPCNODE_LOCALROOT_PREFIX, strlen(CXA_MQTT_RPCNODE_LOCALROOT_PREFIX)) )
	{
		// it's a local request/response/notification and we've reached the top of our hierarchy...start passing it down
		if( superIn->scm_handleMessage_downstream(superIn, topicName, topicNameLen_bytes, msgIn) ) return;

		// if we made it here we couldn't find anything that would handle this message...remap it and toss it up to the server
		if( !cxa_mqtt_message_publish_topicName_trimToPointer(msgIn, topicName+strlen(CXA_MQTT_RPCNODE_LOCALROOT_PREFIX)) ||
			!cxa_mqtt_message_publish_topicName_prependCString(msgIn, "/") ||
			!cxa_mqtt_message_publish_topicName_prependCString(msgIn, nodeIn->super.name) ) return;
	}
	else if( (msgIn != NULL) && cxa_mqtt_client_isConnected(nodeIn->mqttClient) )
	{
		cxa_mqtt_client_publish_message(nodeIn->mqttClient, msgIn);
	}
}


static cxa_mqtt_client_t* scm_getClient(cxa_mqtt_rpc_node_t *const superIn)
{
	cxa_mqtt_rpc_node_root_t* nodeIn = (cxa_mqtt_rpc_node_root_t*)superIn;
	cxa_assert(nodeIn);

	return nodeIn->mqttClient;
}


static cxa_mqtt_rpc_methodRetVal_t rpcMethodCb_isAlive(cxa_mqtt_rpc_node_t *const superIn,
													   cxa_linkedField_t *const paramsIn, cxa_linkedField_t *const returnParamsOut,
													   void* userVarIn)
{
	cxa_mqtt_rpc_node_root_t* nodeIn = (cxa_mqtt_rpc_node_root_t*)superIn;
	cxa_assert(nodeIn);

	cxa_linkedField_append_uint8(returnParamsOut, 37);

	return CXA_MQTT_RPC_METHODRETVAL_SUCCESS;
}
