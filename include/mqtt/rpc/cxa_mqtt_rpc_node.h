/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_MQTT_RPC_NODE_H_
#define CXA_MQTT_RPC_NODE_H_


// ******** includes ********
#include <stdarg.h>
#include <cxa_array.h>
#include <cxa_fixedByteBuffer.h>
#include <cxa_logger_header.h>
#include <cxa_mqtt_client.h>
#include <cxa_timeDiff.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_MQTT_RPCNODE_MAXNUM_SUBNODES
	#define CXA_MQTT_RPCNODE_MAXNUM_SUBNODES				4
#endif

#ifndef CXA_MQTT_RPCNODE_MAXNUM_METHODS
	#define CXA_MQTT_RPCNODE_MAXNUM_METHODS				8
#endif

#ifndef CXA_MQTT_RPCNODE_MAXLEN_NAME_BYTES
	#define CXA_MQTT_RPCNODE_MAXLEN_NAME_BYTES			32
#endif

#ifndef CXA_MQTT_RPCNODE_MAXLEN_METHOD_BYTES
	#define CXA_MQTT_RPCNODE_MAXLEN_METHOD_BYTES			24
#endif

#ifndef CXA_MQTT_RPCNODE_MAXNUM_OUTSTANDING_REQS
	#define CXA_MQTT_RPCNODE_MAXNUM_OUTSTANDING_REQS		2
#endif

#define CXA_MQTT_RPC_VERSION								"v1"
#define CXA_MQTT_RPCNODE_LOCALROOT_PREFIX				"~/"
#define CXA_MQTT_RPCNODE_REQ_PREFIX						"->"
#define CXA_MQTT_RPCNODE_RESP_PREFIX						"<-"
#define CXA_MQTT_RPCNODE_NOTI_PREFIX						"^^"
#define CXA_MQTT_RPCNODE_CONNSTATE_STREAM_NAME			"upstreamConnState"


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_mqtt_rpc_node cxa_mqtt_rpc_node_t;

/**
 * @public
 */
typedef enum
{
	CXA_MQTT_RPC_METHODRETVAL_SUCCESS=0,
	CXA_MQTT_RPC_METHODRETVAL_FAIL_MALFORMED_PATH=1,
	CXA_MQTT_RPC_METHODRETVAL_FAIL_NODE_DNE=2,
	CXA_MQTT_RPC_METHODRETVAL_FAIL_METHOD_DNE=3,
	CXA_MQTT_RPC_METHODRETVAL_FAIL_INVALIDPARAMS=4,
	CXA_MQTT_RPC_METHODRETVAL_FAIL_BAD_STATE=5,
    CXA_MQTT_RPC_METHODRETVAL_FAIL_TIMEOUT=6,
	CXA_MQTT_RPC_METHODRETVAL_FAIL_INTERNAL=255
}cxa_mqtt_rpc_methodRetVal_t;


/**
 * @public
 */
typedef cxa_mqtt_rpc_methodRetVal_t (*cxa_mqtt_rpc_cb_method_t)(cxa_mqtt_rpc_node_t *const nodeIn,
																cxa_linkedField_t *const paramsIn, cxa_linkedField_t *const returnParamsOut,
																void* userVarIn);


/**
 * @public
 */
typedef void (*cxa_mqtt_rpc_cb_methodResponse_t)(cxa_mqtt_rpc_node_t *const nodeIn,
												 cxa_mqtt_rpc_methodRetVal_t retValIn, cxa_linkedField_t *const returnParamsIn,
												 void* userVarIn);


/**
 * @protected
 */
typedef void (*cxa_mqtt_rpc_node_scm_handleMessage_upstream_t)(cxa_mqtt_rpc_node_t *const superIn, cxa_mqtt_message_t *const msgIn);


/**
 * @protected
 */
typedef bool (*cxa_mqtt_rpc_node_scm_handleMessage_downstream_t)(cxa_mqtt_rpc_node_t *const superIn,
																 char *const remainingTopicIn, uint16_t remainingTopicLen_bytesIn,
																 cxa_mqtt_message_t *const msgIn);

/**
 * @protected
 */
typedef cxa_mqtt_client_t* (*cxa_mqtt_rpc_node_scm_getClient_t)(cxa_mqtt_rpc_node_t *const superIn);


/**
 * @private
 */
typedef struct
{
	char name[CXA_MQTT_RPCNODE_MAXLEN_METHOD_BYTES];
	cxa_mqtt_rpc_cb_method_t cb_method;

	void* userVar;
}cxa_mqtt_rpc_node_methodEntry_t;


/**
 * @private
 */
typedef struct
{
	char name[CXA_MQTT_RPCNODE_MAXLEN_METHOD_BYTES];
	char id[5];

	cxa_timeDiff_t td_timeout;

	cxa_mqtt_rpc_cb_methodResponse_t cb;
	void *userVar;
}cxa_mqtt_rpc_node_outstandingRequest_t;


/**
 * @private
 */
struct cxa_mqtt_rpc_node
{
	cxa_mqtt_rpc_node_t* parentNode;
	char name[CXA_MQTT_RPCNODE_MAXLEN_NAME_BYTES];

	cxa_array_t subNodes;
	cxa_mqtt_rpc_node_t* subNodes_raw[CXA_MQTT_RPCNODE_MAXNUM_SUBNODES];

	cxa_array_t methods;
	cxa_mqtt_rpc_node_methodEntry_t methods_raw[CXA_MQTT_RPCNODE_MAXNUM_METHODS];

	cxa_array_t outstandingRequests;
	cxa_mqtt_rpc_node_outstandingRequest_t outstandingRequests_raw[CXA_MQTT_RPCNODE_MAXNUM_OUTSTANDING_REQS];

	cxa_mqtt_rpc_node_scm_handleMessage_upstream_t scm_handleMessage_upstream;
	cxa_mqtt_rpc_node_scm_handleMessage_downstream_t scm_handleMessage_downstream;
	cxa_mqtt_rpc_node_scm_getClient_t scm_getClient;

	cxa_logger_t logger;
};


// ******** global function prototypes ********
/**
 * @public
 */
void cxa_mqtt_rpc_node_init_formattedString(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn, const char *nameFmtIn, ...);


/**
 * @protected
 */
void cxa_mqtt_rpc_node_vinit1(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn,
							  const char *nameFmtIn, va_list varArgsIn);


/**
 * @protected
 */
void cxa_mqtt_rpc_node_vinit2(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn,
							 cxa_mqtt_rpc_node_scm_handleMessage_upstream_t scm_handleMessage_upstreamIn,
							 cxa_mqtt_rpc_node_scm_handleMessage_downstream_t scm_handleMessage_downstreamIn,
							 cxa_mqtt_rpc_node_scm_getClient_t scm_getClientIn,
							 const char *nameFmtIn, va_list varArgsIn);


/**
 * @public
 */
void cxa_mqtt_rpc_node_addMethod(cxa_mqtt_rpc_node_t *const nodeIn, char *const nameIn, cxa_mqtt_rpc_cb_method_t cb_methodIn, void* userVarIn);


/**
 * @public
 */
bool cxa_mqtt_rpc_node_executeMethod(cxa_mqtt_rpc_node_t *const nodeIn,
									 char *const methodNameIn, char *const pathToNodeIn, cxa_fixedByteBuffer_t *const paramsIn,
									 cxa_mqtt_rpc_cb_methodResponse_t responseCbIn, void* userVarIn);


/**
 * @public
 */
bool cxa_mqtt_rpc_node_publishNotification(cxa_mqtt_rpc_node_t *const nodeIn, char *const notiNameIn, cxa_mqtt_qosLevel_t qosIn, void* dataIn, size_t dataSize_bytesIn);
bool cxa_mqtt_rpc_node_publishNotification_appendSubTopic(cxa_mqtt_rpc_node_t *const nodeIn,
														 char *const subTopicIn, char *const notiNameIn, cxa_mqtt_qosLevel_t qosIn,
														 void* dataIn, size_t dataSize_bytesIn);


/**
 * @public
 */
cxa_mqtt_client_t* cxa_mqtt_rpc_node_getClient(cxa_mqtt_rpc_node_t *const nodeIn);


#endif // CXA_MQTT_RPC_NODE_H_
