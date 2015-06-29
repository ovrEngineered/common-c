/**
 * @file
 *
 * @note This object should work across all architecture-specific implementations
 *
 *
 * #### Example Usage: ####
 *
 * @code
 * @endcode
 *
 *
 * @copyright 2013-2014 opencxa.org
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
#ifndef CXA_RPC_NODE_H_
#define CXA_RPC_NODE_H_


// ******** includes ********
#include <stdbool.h>
#include <cxa_array.h>
#include <cxa_logger_header.h>
#include <cxa_rpc_message.h>
#include <cxa_timeBase.h>
#include <cxa_rpc_messageHandler.h>


// ******** global macro definitions ********


#ifndef CXA_RPC_NODE_MAX_NUM_SUBNODES
	#define CXA_RPC_NODE_MAX_NUM_SUBNODES				5
#endif

#ifndef CXA_RPC_NODE_MAX_NUM_METHODS
	#define CXA_RPC_NODE_MAX_NUM_METHODS				5
#endif

#ifndef CXA_RPC_NODE_MAX_METHOD_NAME_LEN_BYTES
	#define CXA_RPC_NODE_MAX_METHOD_NAME_LEN_BYTES		10
#endif

#ifndef CXA_RPC_NODE_MAX_INFLIGHT_SYNC_REQUESTS
	#define CXA_RPC_NODE_MAX_INFLIGHT_SYNC_REQUESTS		2
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef bool (*cxa_rpc_node_method_cb_t)(cxa_linkedField_t *const paramsIn, cxa_linkedField_t *const responseParamsIn, void *userVarIn);


/**
 * @private
 */
typedef struct
{
	char name[CXA_RPC_NODE_MAX_METHOD_NAME_LEN_BYTES+1];

	cxa_rpc_node_method_cb_t cb;
	void* userVar;
}cxa_rpc_node_method_cbEntry_t;


/**
 * @private
 */
typedef struct
{
	CXA_RPC_ID_DATATYPE id;
	cxa_rpc_message_t* response;
}cxa_rpc_node_inflightSyncRequestEntry_t;


/**
 * @private
 */
struct cxa_rpc_node
{
	cxa_rpc_messageHandler_t super;

	bool isGlobalRoot;
	bool isLocalRoot;

	cxa_array_t subnodes;
	cxa_rpc_messageHandler_t* subnodes_raw[CXA_RPC_NODE_MAX_NUM_SUBNODES];

	cxa_array_t methods;
	cxa_rpc_node_method_cbEntry_t methods_raw[CXA_RPC_NODE_MAX_NUM_METHODS];

	cxa_array_t inflightSyncRequests;
	cxa_rpc_node_inflightSyncRequestEntry_t inflightSyncRequests_raw[CXA_RPC_NODE_MAX_INFLIGHT_SYNC_REQUESTS];


	cxa_timeBase_t* timeBase;
	cxa_logger_t logger;

	CXA_RPC_ID_DATATYPE currId;
};


// ******** global function prototypes ********
void cxa_rpc_node_init(cxa_rpc_node_t *const nodeIn, char *const nameIn, cxa_timeBase_t *const timeBaseIn);
void cxa_rpc_node_init_globalRoot(cxa_rpc_node_t *const nodeIn, cxa_timeBase_t *const timeBaseIn);

bool cxa_rpc_node_addSubNode(cxa_rpc_node_t *const nodeIn, cxa_rpc_node_t *const subNodeIn);
bool cxa_rpc_node_addSubNode_remote(cxa_rpc_node_t *const nodeIn, cxa_rpc_nodeRemote_t *const subNodeIn);
bool cxa_rpc_node_addMethod(cxa_rpc_node_t *const nodeIn, char *const nameIn, cxa_rpc_node_method_cb_t cbIn, void* userVarIn);

void cxa_rpc_node_sendMessage_async(cxa_rpc_node_t *const nodeIn, cxa_rpc_message_t* const msgIn);

/**
 * @return the response message...make sure to decrement the reference count when done!
 */
cxa_rpc_message_t* cxa_rpc_node_sendRequest_sync(cxa_rpc_node_t *const nodeIn, cxa_rpc_message_t* const msgIn, uint32_t timeOut_msIn);


#endif // CXA_RPC_NODE_H_
