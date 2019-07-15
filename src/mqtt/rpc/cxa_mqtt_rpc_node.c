/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_mqtt_rpc_node.h"


// ******** includes ********
#include <string.h>
#include <cxa_assert.h>
#include <cxa_mqtt_messageFactory.h>
#include <cxa_mqtt_message_publish.h>
#include <cxa_mqtt_rpc_message.h>
#include <cxa_mqtt_rpc_node_root.h>
#include <cxa_runLoop.h>
#include <cxa_stringUtils.h>

#define CXA_LOG_LEVEL		CXA_LOG_LEVEL_INFO
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********
#define REQUEST_TIMEOUT_MS			3000


// ******** local type definitions ********


// ******** local function prototypes ********
static void scm_handleMessage_upstream(cxa_mqtt_rpc_node_t *const superIn, cxa_mqtt_message_t *const msgIn);
static bool scm_handleRequest_downstream(cxa_mqtt_rpc_node_t *const superIn,
										 char *const remainingTopicIn, uint16_t remainingTopicLen_bytesIn,
										 cxa_mqtt_message_t *const msgIn);
static cxa_mqtt_client_t* scm_getClient(cxa_mqtt_rpc_node_t *const superIn);

static void cb_onRunLoopUpdate(void* userVarIn);

static cxa_mqtt_message_t* prepForResponse(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_message_t* reqMsgIn, cxa_linkedField_t **lf_payloadIn, cxa_linkedField_t **lf_retPayloadIn);
static void sendResponse(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_methodRetVal_t retValIn, cxa_mqtt_message_t *responseMessageIn);

static bool addNodePathToTopic(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_message_t *const msgIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_mqtt_rpc_node_init_formattedString(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn, const char *nameFmtIn, ...)
{
	cxa_assert(nodeIn);
	cxa_assert(parentNodeIn);
	cxa_assert(nameFmtIn);

	va_list varArgs;
	va_start(varArgs, nameFmtIn);
	cxa_mqtt_rpc_node_vinit2(nodeIn, parentNodeIn,
							scm_handleMessage_upstream, scm_handleRequest_downstream, scm_getClient,
							nameFmtIn, varArgs);
	va_end(varArgs);
}


void cxa_mqtt_rpc_node_vinit1(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn,
							  const char *nameFmtIn, va_list varArgsIn)
{
	cxa_assert(nodeIn);
	cxa_assert(parentNodeIn);
	cxa_assert(nameFmtIn);

	cxa_mqtt_rpc_node_vinit2(nodeIn, parentNodeIn,
							 scm_handleMessage_upstream, scm_handleRequest_downstream, scm_getClient,
							 nameFmtIn, varArgsIn);
}


void cxa_mqtt_rpc_node_vinit2(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn,
							 cxa_mqtt_rpc_node_scm_handleMessage_upstream_t scm_handleMessage_upstreamIn,
							 cxa_mqtt_rpc_node_scm_handleMessage_downstream_t scm_handleMessage_downstreamIn,
							 cxa_mqtt_rpc_node_scm_getClient_t scm_getClientIn,
							 const char *nameFmtIn, va_list varArgsIn)
{
	cxa_assert(nodeIn);
	cxa_assert(nameFmtIn);

	// save our references and set some defaults
	nodeIn->parentNode = parentNodeIn;
	nodeIn->scm_handleMessage_upstream = (scm_handleMessage_upstreamIn != NULL) ? scm_handleMessage_upstreamIn : scm_handleMessage_upstream;
	nodeIn->scm_handleMessage_downstream = (scm_handleMessage_downstreamIn != NULL) ? scm_handleMessage_downstreamIn : scm_handleRequest_downstream;
	nodeIn->scm_getClient = (scm_getClientIn != NULL) ? scm_getClientIn : scm_getClient;

	// assemble our name
	cxa_assert(vsnprintf(nodeIn->name, CXA_MQTT_RPCNODE_MAXLEN_NAME_BYTES, nameFmtIn, varArgsIn) < CXA_MQTT_RPCNODE_MAXLEN_NAME_BYTES);
	nodeIn->name[CXA_MQTT_RPCNODE_MAXLEN_NAME_BYTES-1] = 0;

	// setup our subnodes, methods, outstanding requests
	cxa_array_initStd(&nodeIn->subNodes, nodeIn->subNodes_raw);
	cxa_array_initStd(&nodeIn->methods, nodeIn->methods_raw);
	cxa_array_initStd(&nodeIn->outstandingRequests, nodeIn->outstandingRequests_raw);

	// setup our logger
	cxa_logger_init_formattedString(&nodeIn->logger, "mRpcNode_%s", nodeIn->name);

	// add as a subnode (if we have a parent)
	if( nodeIn->parentNode != NULL ) cxa_assert( cxa_array_append(&nodeIn->parentNode->subNodes, (void*)&nodeIn) );

	// register for run loop execution
	cxa_mqtt_client_t* mqttClient = cxa_mqtt_rpc_node_getClient(nodeIn);
	cxa_assert(mqttClient);
	cxa_runLoop_addEntry(cxa_mqtt_client_getThreadId(mqttClient), NULL, cb_onRunLoopUpdate, (void*)nodeIn);
}


void cxa_mqtt_rpc_node_addMethod(cxa_mqtt_rpc_node_t *const nodeIn, char *const nameIn, cxa_mqtt_rpc_cb_method_t cb_methodIn, void* userVarIn)
{
	cxa_assert(nodeIn);
	cxa_assert(cb_methodIn);

	cxa_mqtt_rpc_node_methodEntry_t newEntry = {
		.cb_method = cb_methodIn,
		.userVar = userVarIn
	};
	cxa_assert( nameIn && (strlen(nameIn) < (sizeof(newEntry.name)-1)) );
	cxa_stringUtils_copy(newEntry.name, nameIn, sizeof(newEntry.name));
	cxa_assert( cxa_array_append(&nodeIn->methods, &newEntry) );
}


bool cxa_mqtt_rpc_node_executeMethod(cxa_mqtt_rpc_node_t *const nodeIn,
									 char *const methodNameIn, char *const pathToNodeIn, cxa_fixedByteBuffer_t *const paramsIn,
									 cxa_mqtt_rpc_cb_methodResponse_t responseCbIn, void* userVarIn)
{
	cxa_assert(nodeIn);
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
	uint16_t sentReqId = currRequestId++;
	snprintf(msgId, sizeof(msgId), "%04X", sentReqId);
	msgId[4] = 0;
	if( !cxa_mqtt_message_publish_topicName_prependCString(msg, msgId) ||
		!cxa_mqtt_message_publish_topicName_prependCString(msg, "/") )
	{
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}

	// now the method name
	if( (strlen(methodNameIn) >= CXA_MQTT_RPCNODE_MAXLEN_METHOD_BYTES) ||
		!cxa_mqtt_message_publish_topicName_prependCString(msg, methodNameIn) ||
		!cxa_mqtt_message_publish_topicName_prependCString(msg, CXA_MQTT_RPCNODE_REQ_PREFIX) ||
		((pathToNodeIn != NULL) && !cxa_mqtt_message_publish_topicName_prependCString(msg, "/")) )
	{
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}

	// now the path to the node
	if( (pathToNodeIn != NULL) && !cxa_mqtt_message_publish_topicName_prependCString(msg, pathToNodeIn) )
	{
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}

	// make sure we can get the topic name for the message before we go further
	char* remainingTopic;
	uint16_t remainingTopicLen_bytes;
	if( !cxa_mqtt_message_publish_getTopicName(msg, &remainingTopic, &remainingTopicLen_bytes) )
	{
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}

	// good, now add an outstanding request entry for this message (if desired)
	if( responseCbIn != NULL )
	{
		cxa_mqtt_rpc_node_outstandingRequest_t newRequest = {
				.cb = responseCbIn,
				.userVar = userVarIn
		};

		// copy over the method name and the id
		cxa_stringUtils_copy(newRequest.name, methodNameIn, sizeof(newRequest.name));
		cxa_stringUtils_copy(newRequest.id, msgId, sizeof(newRequest.id));

		// initialize our timeout
		cxa_timeDiff_init(&newRequest.td_timeout);

		// add to our list of outstanding requests
		if( !cxa_array_append(&nodeIn->outstandingRequests, &newRequest) )
		{
			cxa_logger_warn(&nodeIn->logger, "too many outstanding requests, dropping");
			cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
			return false;
		}
	}

	// excellent...now we need to figure out where this message is headed...
	if( (pathToNodeIn != NULL) &&
		(cxa_stringUtils_startsWith(pathToNodeIn, "/") || cxa_stringUtils_startsWith(pathToNodeIn, "~/")) )
	{
		// this message is addressed from the global root or the
		// local root respectively...send it up!
		nodeIn->scm_handleMessage_upstream(nodeIn, msg);
	}
	else
	{
		// this message is addressed locally...therefore it must be headed
		// for one of _our_ subnodes...send it down!
		nodeIn->scm_handleMessage_downstream(nodeIn, remainingTopic, remainingTopicLen_bytes, msg);
	}

	// release our sent message
	cxa_mqtt_messageFactory_decrementMessageRefCount(msg);

	return true;
}


bool cxa_mqtt_rpc_node_publishNotification(cxa_mqtt_rpc_node_t *const nodeIn, char *const notiNameIn, cxa_mqtt_qosLevel_t qosIn, void* dataIn, size_t dataSize_bytesIn)
{
	cxa_assert(nodeIn);
	return cxa_mqtt_rpc_node_publishNotification_appendSubTopic(nodeIn, NULL, notiNameIn, qosIn, dataIn, dataSize_bytesIn);
}


bool cxa_mqtt_rpc_node_publishNotification_appendSubTopic(cxa_mqtt_rpc_node_t *const nodeIn,
														 char *const subTopicIn, char *const notiNameIn, cxa_mqtt_qosLevel_t qosIn,
														 void* dataIn, size_t dataSize_bytesIn)
{
	cxa_assert(nodeIn);
	cxa_assert(notiNameIn);

	// first, we need to form our message
	cxa_mqtt_message_t* msg = cxa_mqtt_messageFactory_getFreeMessage_empty();
	if( (msg == NULL) ||
		!cxa_mqtt_message_publish_init(msg, false, CXA_MQTT_QOS_ATMOST_ONCE, false,
									  "", 0, dataIn, dataSize_bytesIn) )
	{
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}

	// now we need to get our topic/path in order...start with the notification info
	if( !cxa_mqtt_message_publish_topicName_prependCString(msg, notiNameIn) ||
		!cxa_mqtt_message_publish_topicName_prependCString(msg, "/") )
	{
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}
	// now add the subTopic if desired
	if( (subTopicIn != NULL) &&
		(!cxa_mqtt_message_publish_topicName_prependCString(msg, subTopicIn) ||
		 !cxa_mqtt_message_publish_topicName_prependCString(msg, "/") ) )
	{
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}
	// and the rest of our path
	if( !addNodePathToTopic(nodeIn, msg) )
	{
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}
	// and the message type and version
	if( !cxa_mqtt_message_publish_topicName_prependCString(msg, "/") ||
		!cxa_mqtt_message_publish_topicName_prependCString(msg, CXA_MQTT_RPCNODE_NOTI_PREFIX) ||
		!cxa_mqtt_message_publish_topicName_prependCString(msg, "/") ||
		!cxa_mqtt_message_publish_topicName_prependCString(msg, "v1") )
	{
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}


	// make sure we can get the topic name for the message before we go further
	char* remainingTopic;
	uint16_t remainingTopicLen_bytes;
	if( !cxa_mqtt_message_publish_getTopicName(msg, &remainingTopic, &remainingTopicLen_bytes) )
	{
		cxa_mqtt_messageFactory_decrementMessageRefCount(msg);
		return false;
	}

	// excellent...this is a notification so it's always headed upstream
	nodeIn->scm_handleMessage_upstream(nodeIn, msg);

	// release our sent message
	cxa_mqtt_messageFactory_decrementMessageRefCount(msg);

	return true;
}


cxa_mqtt_client_t* cxa_mqtt_rpc_node_getClient(cxa_mqtt_rpc_node_t *const nodeIn)
{
	cxa_assert(nodeIn);
	cxa_assert(nodeIn->scm_getClient != NULL);

	return nodeIn->scm_getClient(nodeIn);
}


// ******** local function implementations ********
static void scm_handleMessage_upstream(cxa_mqtt_rpc_node_t *const superIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(superIn);
	cxa_assert(msgIn);

	char* topicName;
	uint16_t topicNameLen_bytes;
	if( !cxa_mqtt_message_publish_getTopicName(msgIn, &topicName, &topicNameLen_bytes) ) return;

//	cxa_logger_log_untermString(&superIn->logger, CXA_LOG_LEVEL_TRACE, "<< '", topicName, topicNameLen_bytes, "'");

//	if( cxa_stringUtils_startsWith_withLengths(topicName, topicNameLen_bytes, "/", 1) ||
//		cxa_stringUtils_startsWith_withLengths(topicName, topicNameLen_bytes, CXA_MQTT_RPCNODE_LOCALROOT_PREFIX, strlen(CXA_MQTT_RPCNODE_LOCALROOT_PREFIX)))
//	{
		// this message is addressed from the global root or the local root respectively...send it up!
		if( superIn->parentNode != NULL ) superIn->parentNode->scm_handleMessage_upstream(superIn->parentNode, msgIn);
//	}
//	else
//	{
//		// this message is non-root relative...that means it probably originated someplace system-local
//		// @TODO implement this
//	}
}


static bool scm_handleRequest_downstream(cxa_mqtt_rpc_node_t *const superIn,
										 char *const remainingTopicIn, uint16_t remainingTopicLen_bytesIn,
										 cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(superIn);
	cxa_assert(remainingTopicIn);
	cxa_assert(msgIn);

	cxa_logger_trace_untermString(&superIn->logger, ">> '", remainingTopicIn, remainingTopicLen_bytesIn, "'");

	// make sure that the topic starts with our name (handle the special "localroot" case)
	size_t nodeNameLen_bytes = strlen(superIn->name);
	if( superIn->parentNode != NULL )
	{
		// not the local root node
		if( !cxa_stringUtils_startsWith_withLengths(remainingTopicIn, remainingTopicLen_bytesIn, superIn->name, nodeNameLen_bytes) ) return false;
	}
	else
	{
		// we are the local root node...check for name variants
		if( cxa_stringUtils_startsWith_withLengths(remainingTopicIn, remainingTopicLen_bytesIn, CXA_MQTT_RPCNODE_LOCALROOT_PREFIX, strlen(CXA_MQTT_RPCNODE_LOCALROOT_PREFIX)) )
		{
			nodeNameLen_bytes = strlen(CXA_MQTT_RPCNODE_LOCALROOT_PREFIX);
		}
		else if( !cxa_stringUtils_startsWith_withLengths(remainingTopicIn, remainingTopicLen_bytesIn, superIn->name, nodeNameLen_bytes) ) return false;
	}

	// so far so good...remove ourselves from the topic
	char* currTopic = remainingTopicIn + nodeNameLen_bytes;
	size_t currTopicLen_bytes = remainingTopicLen_bytesIn - nodeNameLen_bytes;

	// if there is a remaining separator, remove it
	if( cxa_stringUtils_startsWith_withLengths(currTopic, currTopicLen_bytes, "/", 1) )
	{
		currTopic++;
		currTopicLen_bytes--;
	}

	// count our remaining separators to tell us if the message is bound for one of our methods
	if( cxa_stringUtils_countOccurences_withLengths(currTopic, currTopicLen_bytes, "/", 1) == 0 )
	{
		// no more separators...start looking for a method
		cxa_array_iterate(&superIn->methods, currMethodEntry, cxa_mqtt_rpc_node_methodEntry_t)
		{
			if( currMethodEntry == NULL ) continue;

			if( cxa_stringUtils_startsWith_withLengths(currTopic, currTopicLen_bytes, currMethodEntry->name, strlen(currMethodEntry->name)) )
			{
				cxa_logger_trace(&superIn->logger, "found method '%s'", currMethodEntry->name);

				// if we made it here we'll be sending a response
				cxa_linkedField_t *lf_payload, *lf_retPayload;
				cxa_mqtt_message_t* respMsg = prepForResponse(superIn, msgIn, &lf_payload, &lf_retPayload);
				if( respMsg == NULL ) return true;

				cxa_mqtt_rpc_methodRetVal_t retVal = CXA_MQTT_RPC_METHODRETVAL_SUCCESS;
				if( currMethodEntry->cb_method != NULL ) retVal = currMethodEntry->cb_method(superIn, lf_payload, lf_retPayload, currMethodEntry->userVar);
				sendResponse(superIn, retVal, respMsg);

				return true;
			}
		}

		// if we made it here, it is bound for a unknown method
		cxa_logger_warn_untermString(&superIn->logger, "unknown method: '", currTopic, currTopicLen_bytes, "'");
		cxa_linkedField_t *lf_payload, *lf_retPayload;
		cxa_mqtt_message_t* respMsg = prepForResponse(superIn, msgIn, &lf_payload, &lf_retPayload);
		if( respMsg != NULL ) sendResponse(superIn, CXA_MQTT_RPC_METHODRETVAL_FAIL_METHOD_DNE, respMsg);

		return true;
	}

	// if we made it here...this must be destined for a subnode
	cxa_array_iterate(&superIn->subNodes, currSubNode, cxa_mqtt_rpc_node_t*)
	{
		if( currSubNode == NULL ) continue;
		if( ((*currSubNode)->scm_handleMessage_downstream != NULL) && (*currSubNode)->scm_handleMessage_downstream(*currSubNode, currTopic, currTopicLen_bytes, msgIn) ) return true;
	}

	// if we made it here, it is bound for an unknown subnode
	cxa_logger_warn_untermString(&superIn->logger, "unknown subNode: '", currTopic, currTopicLen_bytes, "'");
	cxa_linkedField_t *lf_payload, *lf_retPayload;
	cxa_mqtt_message_t* respMsg = prepForResponse(superIn, msgIn, &lf_payload, &lf_retPayload);
	if( respMsg != NULL ) sendResponse(superIn, CXA_MQTT_RPC_METHODRETVAL_FAIL_NODE_DNE, respMsg);
	return true;
}


static cxa_mqtt_client_t* scm_getClient(cxa_mqtt_rpc_node_t *const superIn)
{
	return (superIn->parentNode != NULL) ? cxa_mqtt_rpc_node_getClient(superIn->parentNode) : NULL;
}


static void cb_onRunLoopUpdate(void* userVarIn)
{
	cxa_mqtt_rpc_node_t* nodeIn = (cxa_mqtt_rpc_node_t*)userVarIn;
	cxa_assert(nodeIn);

	// check our outstanding requests for timeouts
	cxa_array_iterate(&nodeIn->outstandingRequests, currRequest, cxa_mqtt_rpc_node_outstandingRequest_t)
	{
		if( currRequest == NULL ) continue;

		if( cxa_timeDiff_isElapsed_ms(&currRequest->td_timeout, REQUEST_TIMEOUT_MS) )
		{
			currRequest->cb(nodeIn, CXA_MQTT_RPC_METHODRETVAL_FAIL_TIMEOUT, NULL, currRequest->userVar);
			cxa_array_remove(&nodeIn->outstandingRequests, currRequest);
			break;
		}
	}

	// iterate through our subnodes and update them as well
	cxa_array_iterate(&nodeIn->subNodes, currSubNode, cxa_mqtt_rpc_node_t*)
	{
		if( currSubNode == NULL ) continue;
		cb_onRunLoopUpdate((void*)*currSubNode);
	}
}


static cxa_mqtt_message_t* prepForResponse(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_message_t* reqMsgIn, cxa_linkedField_t **lf_payloadIn, cxa_linkedField_t **lf_retPayloadIn)
{
	cxa_assert(nodeIn);
	cxa_assert(reqMsgIn);
	cxa_assert(lf_payloadIn);
	cxa_assert(lf_retPayloadIn);

	char *reqTopicName, *respTopicName;
	uint16_t reqTopicNameLen_bytes, respTopicNameLen_bytes;

	cxa_mqtt_message_t* respMsg = cxa_mqtt_messageFactory_getFreeMessage_empty();
	if( (respMsg == NULL) || !cxa_mqtt_message_publish_init(respMsg, false, CXA_MQTT_QOS_ATMOST_ONCE, false, "", 0, NULL, 0) ||				// get the response and init it
		!cxa_mqtt_message_publish_getTopicName(reqMsgIn, &reqTopicName, &reqTopicNameLen_bytes) ||											// setup the response topic
		!cxa_mqtt_message_publish_topicName_prependString_withLength(respMsg, reqTopicName, reqTopicNameLen_bytes) ||
		!cxa_mqtt_message_publish_getTopicName(respMsg, &respTopicName, &respTopicNameLen_bytes) ||
		!cxa_stringUtils_replaceFirstOccurence_withLengths(respTopicName, respTopicNameLen_bytes,
														   CXA_MQTT_RPCNODE_REQ_PREFIX, strlen(CXA_MQTT_RPCNODE_REQ_PREFIX),
														   CXA_MQTT_RPCNODE_RESP_PREFIX, strlen(CXA_MQTT_RPCNODE_RESP_PREFIX)) ||
		!cxa_mqtt_message_publish_getPayload(reqMsgIn, lf_payloadIn) || !cxa_mqtt_message_publish_getPayload(respMsg, lf_retPayloadIn) )	// setup payloads
	{
		cxa_logger_warn(&nodeIn->logger, "error reserving/init'ing response");
		return NULL;
	}

	return respMsg;
}


static void sendResponse(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_methodRetVal_t retValIn, cxa_mqtt_message_t *responseMessageIn)
{
	cxa_assert(nodeIn);
	cxa_assert(responseMessageIn);

	cxa_linkedField_t* lf_retPayload;
	if( cxa_mqtt_message_publish_getPayload(responseMessageIn, &lf_retPayload) &&
		cxa_linkedField_insert_uint8(lf_retPayload, 0, (uint8_t)retValIn) )
	{
		nodeIn->scm_handleMessage_upstream(nodeIn, responseMessageIn);
	}else cxa_logger_warn(&nodeIn->logger, "error sending response");
	cxa_mqtt_messageFactory_decrementMessageRefCount(responseMessageIn);
}


static bool addNodePathToTopic(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_message_t *const msgIn)
{
	cxa_assert(nodeIn);
	cxa_assert(msgIn);

	// add ourselves first
	if( !cxa_mqtt_message_publish_topicName_prependCString(msgIn, nodeIn->name) ) return false;

	// add our parent if it exists
	if( (nodeIn->parentNode != NULL) &&
		( !cxa_mqtt_message_publish_topicName_prependCString(msgIn, "/") ||
		  !addNodePathToTopic(nodeIn->parentNode, msgIn)) ) return false;

	return true;
}
