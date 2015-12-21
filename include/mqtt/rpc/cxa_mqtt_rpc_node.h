/**
 * @file
 * @copyright 2015 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
	#define CXA_MQTT_RPCNODE_MAXNUM_METHODS					8
#endif

#ifndef CXA_MQTT_RPCNODE_MAXLEN_RETURNPARAMS_BYTES
	#define CXA_MQTT_RPCNODE_MAXLEN_RETURNPARAMS_BYTES		64
#endif

#ifndef CXA_MQTT_RPCNODE_MAXLEN_NAME_BYTES
	#define CXA_MQTT_RPCNODE_MAXLEN_NAME_BYTES				16
#endif

#ifndef CXA_MQTT_RPCNODE_MAXLEN_METHOD_BYTES
	#define CXA_MQTT_RPCNODE_MAXLEN_METHOD_BYTES			24
#endif

#define CXA_MQTT_RPCNODE_REQ_PREFIX							"::"
#define CXA_MQTT_RPCNODE_RESP_PREFIX						"/rpcResp"


// ******** global type definitions *********
/**
 * @public
 */
typedef struct cxa_mqtt_rpc_node cxa_mqtt_rpc_node_t;


/**
 * @public
 */
typedef struct cxa_mqtt_rpc_node_root cxa_mqtt_rpc_node_root_t;


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
	CXA_MQTT_RPC_METHODRETVAL_FAIL_INTERNAL=255
}cxa_mqtt_rpc_methodRetVal_t;


/**
 * @public
 */
typedef cxa_mqtt_rpc_methodRetVal_t (*cxa_mqtt_rpc_cb_method_t)(cxa_mqtt_rpc_node_t *const nodeIn,
																cxa_fixedByteBuffer_t *const paramsIn, cxa_fixedByteBuffer_t *const returnParamsOut,
																void* userVarIn);


/**
 * @public
 */
typedef bool (*cxa_mqtt_rpc_cb_catchall_t)(cxa_mqtt_rpc_node_t *const nodeIn,
											char *const remainingTopicIn, size_t remainingTopicLen_bytes,
											cxa_mqtt_message_t *const msgIn, void* userVarIn);


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
struct cxa_mqtt_rpc_node
{
	cxa_mqtt_rpc_node_t* parentNode;
	char name[CXA_MQTT_RPCNODE_MAXLEN_NAME_BYTES];
	bool isRootNode;

	cxa_array_t subNodes;
	cxa_mqtt_rpc_node_t* subNodes_raw[CXA_MQTT_RPCNODE_MAXNUM_SUBNODES];

	cxa_array_t methods;
	cxa_mqtt_rpc_node_methodEntry_t methods_raw[CXA_MQTT_RPCNODE_MAXNUM_METHODS];

	cxa_mqtt_rpc_cb_catchall_t cb_catchall;
	void* catchAll_userVar;

	cxa_logger_t logger;
};


// ******** global function prototypes ********
/**
 * @protected
 */
void cxa_mqtt_rpc_node_vinit(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn, const char *nameFmtIn, va_list varArgsIn);


/**
 * @public
 */
void cxa_mqtt_rpc_node_addMethod(cxa_mqtt_rpc_node_t *const nodeIn, char *const nameIn, cxa_mqtt_rpc_cb_method_t cb_methodIn, void* userVarIn);


/**
 * @protected
 */
void cxa_mqtt_rpc_node_setCatchAll(cxa_mqtt_rpc_node_t *const nodeIn, cxa_mqtt_rpc_cb_catchall_t cb_catchallIn, void *userVarIn);


/**
 * @public
 */
bool cxa_mqtt_rpc_node_publishNotification(cxa_mqtt_rpc_node_t *const nodeIn, char *const notiNameIn, cxa_mqtt_qosLevel_t qosIn, void* dataIn, size_t dataSize_bytesIn);


/**
 * @protected
 */
bool cxa_mqtt_rpc_node_getTopicForNode(cxa_mqtt_rpc_node_t *const nodeIn, char* topicOut, size_t maxTopicLen_bytesIn);


/**
 * @protected
 */
cxa_mqtt_rpc_node_root_t* cxa_mqtt_rpc_node_getRootNode(cxa_mqtt_rpc_node_t *const nodeIn);


#endif // CXA_MQTT_RPC_NODE_H_
